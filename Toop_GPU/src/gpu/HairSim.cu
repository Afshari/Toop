#include "gpu/HairSim.cuh"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace Toop {

	// --------------------------------------------------------------------------------
	// DEVICE KERNELS
	// --------------------------------------------------------------------------------

	__global__ void k_solve_ground_collision(
		float* __restrict__ pos_y,
		const float* __restrict__ inv_mass,
		int   total_particles,
		float ground_y)
	{
		int idx = blockIdx.x * blockDim.x + threadIdx.x;
		if (idx >= total_particles) return;
		if (inv_mass[idx] == 0.0f)  return;   // skip roots
		if (pos_y[idx] < ground_y)
			pos_y[idx] = ground_y;
	}

	__global__ void k_solve_sphere_collision(
		float* __restrict__ pos_x,
		float* __restrict__ pos_y,
		float* __restrict__ pos_z,
		const float* __restrict__ inv_mass,
		int    total_particles,
		float  sphere_cx,
		float  sphere_cy,
		float  sphere_cz,
		float  sphere_radius,
		float  compliance,
		float  sub_dt)
	{
		int idx = blockIdx.x * blockDim.x + threadIdx.x;
		if (idx >= total_particles) return;

		float w = inv_mass[idx];
		if (w == 0.0f) return;   // skip roots

		float dx = pos_x[idx] - sphere_cx;
		float dy = pos_y[idx] - sphere_cy;
		float dz = pos_z[idx] - sphere_cz;

		float dist = sqrtf(dx * dx + dy * dy + dz * dz);

		if (dist >= sphere_radius || dist < 1e-6f) return;

		float C = dist - sphere_radius;
		float alpha_tilde = compliance / (sub_dt * sub_dt);
		float d_lambda = -C / (w + alpha_tilde);

		float inv_dist = 1.0f / dist;
		pos_x[idx] += w * d_lambda * dx * inv_dist;
		pos_y[idx] += w * d_lambda * dy * inv_dist;
		pos_z[idx] += w * d_lambda * dz * inv_dist;
	}

	__global__ void k_integrate(
		float* __restrict__ pos_x,
		float* __restrict__ pos_y,
		float* __restrict__ pos_z,
		float* __restrict__ prev_pos_x,
		float* __restrict__ prev_pos_y,
		float* __restrict__ prev_pos_z,
		const float* __restrict__ inv_mass,
		int   total_particles,
		float gravity_y,
		float damping,
		float dt,
		float wind_x,
		float wind_y,
		float wind_z,
		float time)
	{
		int idx = blockIdx.x * blockDim.x + threadIdx.x;
		if (idx >= total_particles) return;
		if (inv_mass[idx] == 0.0f) return;

		float vx = (pos_x[idx] - prev_pos_x[idx]) / dt;
		float vy = (pos_y[idx] - prev_pos_y[idx]) / dt;
		float vz = (pos_z[idx] - prev_pos_z[idx]) / dt;

		float phase = (idx % 128) * 0.11f + (idx / 128) * 0.17f;
		float noise1 = sinf(phase + time * 2.1f) * cosf(phase * 0.53f + time * 1.3f);
		float noise2 = sinf(phase * 1.7f + time * 3.7f) * 0.5f;
		float noise3 = cosf(phase * 2.9f + time * 0.7f) * 0.3f;
		float noise = (noise1 + noise2 + noise3) / 1.8f;
		float wind_scale = 0.4f + 12.0f * noise;

		vx += wind_x * wind_scale * dt;
		vy += gravity_y * dt;
		vz += wind_z * wind_scale * dt;

		vx *= damping;
		vy *= damping;
		vz *= damping;

		prev_pos_x[idx] = pos_x[idx];
		prev_pos_y[idx] = pos_y[idx];
		prev_pos_z[idx] = pos_z[idx];

		pos_x[idx] += vx * dt;
		pos_y[idx] += vy * dt;
		pos_z[idx] += vz * dt;
	}

	__global__ void k_update_roots(
		float* __restrict__ pos_x,
		float* __restrict__ pos_y,
		float* __restrict__ pos_z,
		float* __restrict__ prev_pos_x,
		float* __restrict__ prev_pos_y,
		float* __restrict__ prev_pos_z,
		const float3* __restrict__ root_dirs,
		int   num_strands,
		int   particles_per_strand,
		float sphere_cx,
		float sphere_cy,
		float sphere_cz,
		float sphere_radius,
		float qx, float qy, float qz, float qw)
	{
		int sid = blockIdx.x * blockDim.x + threadIdx.x;
		if (sid >= num_strands) return;

		// rotate root_dir by quaternion (Rodrigues formula)
		float3 v = root_dirs[sid];

		float cx = qy * v.z - qz * v.y;
		float cy = qz * v.x - qx * v.z;
		float cz = qx * v.y - qy * v.x;

		float rx = v.x + 2.0f * qw * cx + 2.0f * (qy * cz - qz * cy);
		float ry = v.y + 2.0f * qw * cy + 2.0f * (qz * cx - qx * cz);
		float rz = v.z + 2.0f * qw * cz + 2.0f * (qx * cy - qy * cx);

		float root_x = sphere_cx + rx * sphere_radius;
		float root_y = sphere_cy + ry * sphere_radius;
		float root_z = sphere_cz + rz * sphere_radius;

		// particle-major layout: segment 0 of strand sid is at index sid
		int root_idx = sid;

		// update both pos and prev_pos to avoid velocity spike at root
		pos_x[root_idx] = root_x;
		pos_y[root_idx] = root_y;
		pos_z[root_idx] = root_z;

		prev_pos_x[root_idx] = root_x;
		prev_pos_y[root_idx] = root_y;
		prev_pos_z[root_idx] = root_z;
	}

	__global__ void k_solve_constraints_xpbd(
		float* __restrict__ pos_x,
		float* __restrict__ pos_y,
		float* __restrict__ pos_z,
		const float* __restrict__ inv_mass,
		const float* __restrict__ rest_lengths,
		float* __restrict__ lambdas,
		int   num_strands,
		int   num_segments,
		int   particles_per_strand,
		float compliance,
		float sub_dt)
	{
		int sid = blockIdx.x * blockDim.x + threadIdx.x;
		if (sid >= num_strands) return;

		float alpha_tilde = compliance / (sub_dt * sub_dt);
		int   lambda_base = sid * num_segments;

		for (int i = 0; i < num_segments; i++)
		{
			// particle-major layout: particle i of strand sid
			// is at index i * num_strands + sid
			int id0 = i * num_strands + sid;
			int id1 = (i + 1) * num_strands + sid;

			float dx = pos_x[id1] - pos_x[id0];
			float dy = pos_y[id1] - pos_y[id0];
			float dz = pos_z[id1] - pos_z[id0];

			float dist = sqrtf(dx * dx + dy * dy + dz * dz);
			if (dist < 1e-6f) continue;

			float w0 = inv_mass[id0];
			float w1 = inv_mass[id1];
			float w_sum = w0 + w1;
			if (w_sum < 1e-6f) continue;

			float C = dist - rest_lengths[lambda_base + i];
			float lambda = lambdas[lambda_base + i];
			float d_lambda = (-C - alpha_tilde * lambda) / (w_sum + alpha_tilde);

			lambdas[lambda_base + i] += d_lambda;

			float inv_dist = 1.0f / dist;
			float nx = dx * inv_dist;
			float ny = dy * inv_dist;
			float nz = dz * inv_dist;

			pos_x[id0] -= w0 * d_lambda * nx;
			pos_y[id0] -= w0 * d_lambda * ny;
			pos_z[id0] -= w0 * d_lambda * nz;

			pos_x[id1] += w1 * d_lambda * nx;
			pos_y[id1] += w1 * d_lambda * ny;
			pos_z[id1] += w1 * d_lambda * nz;
		}
	}

	__global__ void k_init_rand_states(
		curandState* __restrict__ states,
		int total_particles,
		unsigned long long seed)
	{
		int idx = blockIdx.x * blockDim.x + threadIdx.x;
		if (idx >= total_particles) return;
		curand_init(seed, idx, 0, &states[idx]);
	}

	__global__ void k_translate_with_perturbation(
		float* __restrict__ pos_x,
		float* __restrict__ pos_y,
		float* __restrict__ pos_z,
		float* __restrict__ prev_pos_x,
		float* __restrict__ prev_pos_y,
		float* __restrict__ prev_pos_z,
		const float* __restrict__ inv_mass,
		curandState* __restrict__ states,
		int   total_particles,
		int   particles_per_strand,
		float delta_x,
		float delta_y,
		float delta_z,
		float drag_speed)
	{
		int idx = blockIdx.x * blockDim.x + threadIdx.x;
		if (idx >= total_particles) return;
		if (inv_mass[idx] == 0.0f) return;

		// particle-major layout: segment index = idx / num_strands
		int   num_strands_local = total_particles / particles_per_strand;
		int   segment = idx / num_strands_local;
		float t = (float)segment / (float)(particles_per_strand - 1);
		float move_factor = 1.0f - t * 0.3f;

		float dx = delta_x * move_factor;
		float dy = delta_y * move_factor;
		float dz = delta_z * move_factor;

		pos_x[idx] += dx;
		pos_y[idx] += dy;
		pos_z[idx] += dz;

		prev_pos_x[idx] += dx;
		prev_pos_y[idx] += dy;
		prev_pos_z[idx] += dz;
	}

	__global__ void k_pack_positions_aos(
		const float* __restrict__ pos_x,
		const float* __restrict__ pos_y,
		const float* __restrict__ pos_z,
		float* __restrict__ aos_buffer,
		int total_particles,
		int num_strands,
		int particles_per_strand)
	{
		int idx = blockIdx.x * blockDim.x + threadIdx.x;
		if (idx >= total_particles) return;

		// idx is the output index in strand-major order
		// strand-major: idx = sid * particles_per_strand + i
		int sid = idx / particles_per_strand;
		int i = idx % particles_per_strand;

		// particle-major source index: i * num_strands + sid
		int src = i * num_strands + sid;

		aos_buffer[idx * 3 + 0] = pos_x[src];
		aos_buffer[idx * 3 + 1] = pos_y[src];
		aos_buffer[idx * 3 + 2] = pos_z[src];
	}

	__global__ void k_apply_drag_velocity(
		float* __restrict__ pos_x,
		float* __restrict__ pos_y,
		float* __restrict__ pos_z,
		const float* __restrict__ prev_pos_x,
		const float* __restrict__ prev_pos_y,
		const float* __restrict__ prev_pos_z,
		const float* __restrict__ inv_mass,
		int   total_particles,
		float vel_x,
		float vel_y,
		float vel_z,
		float scale)
	{
		int idx = blockIdx.x * blockDim.x + threadIdx.x;
		if (idx >= total_particles) return;
		if (inv_mass[idx] == 0.0f) return;

		// implicit velocity = (pos - prev_pos) / dt
		pos_x[idx] += vel_x * scale;
		pos_y[idx] += vel_y * scale;
		pos_z[idx] += vel_z * scale;
	}

	__global__ void k_update_velocity(
		const float* __restrict__ pos_x,
		const float* __restrict__ pos_y,
		const float* __restrict__ pos_z,
		const float* __restrict__ prev_pos_x,
		const float* __restrict__ prev_pos_y,
		const float* __restrict__ prev_pos_z,
		float* __restrict__ vel_x,
		float* __restrict__ vel_y,
		float* __restrict__ vel_z,
		const float* __restrict__ inv_mass,
		int   total_particles,
		float inv_dt)
	{
		int idx = blockIdx.x * blockDim.x + threadIdx.x;
		if (idx >= total_particles) return;
		if (inv_mass[idx] == 0.0f) return;

		vel_x[idx] = (pos_x[idx] - prev_pos_x[idx]) * inv_dt;
		vel_y[idx] = (pos_y[idx] - prev_pos_y[idx]) * inv_dt;
		vel_z[idx] = (pos_z[idx] - prev_pos_z[idx]) * inv_dt;
	}

	// --------------------------------------------------------------------------------
	// HOST LAUNCHERS
	// --------------------------------------------------------------------------------

	void launch_solve_ground_collision(
		float* pos_y,
		const float* inv_mass,
		int   total_particles,
		float ground_y,
		int   threads_per_block)
	{
		int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
		k_solve_ground_collision << <blocks, threads_per_block >> > (
			pos_y, inv_mass, total_particles, ground_y);
	}

	void launch_solve_sphere_collision(
		float* pos_x,
		float* pos_y,
		float* pos_z,
		const float* inv_mass,
		int    total_particles,
		float  sphere_cx,
		float  sphere_cy,
		float  sphere_cz,
		float  sphere_radius,
		float  compliance,
		float  sub_dt,
		int    threads_per_block)
	{
		int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
		k_solve_sphere_collision << <blocks, threads_per_block >> > (
			pos_x, pos_y, pos_z, inv_mass,
			total_particles,
			sphere_cx, sphere_cy, sphere_cz,
			sphere_radius, compliance, sub_dt);
	}

	void launch_integrate(
		float* pos_x,
		float* pos_y,
		float* pos_z,
		float* prev_pos_x,
		float* prev_pos_y,
		float* prev_pos_z,
		const float* inv_mass,
		int   total_particles,
		float gravity_y,
		float damping,
		float dt,
		float wind_x,
		float wind_y,
		float wind_z,
		float time,
		int   threads_per_block)
	{
		int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
		k_integrate << <blocks, threads_per_block >> > (
			pos_x, pos_y, pos_z,
			prev_pos_x, prev_pos_y, prev_pos_z,
			inv_mass, total_particles, gravity_y, damping, dt,
			wind_x, wind_y, wind_z, time);
	}

	void launch_update_roots(
		float* pos_x,
		float* pos_y,
		float* pos_z,
		float* prev_pos_x,
		float* prev_pos_y,
		float* prev_pos_z,
		const float3* root_dirs,
		int   num_strands,
		int   particles_per_strand,
		float sphere_cx,
		float sphere_cy,
		float sphere_cz,
		float sphere_radius,
		float qx, float qy, float qz, float qw,
		int   threads_per_block)
	{
		int blocks = (num_strands + threads_per_block - 1) / threads_per_block;
		k_update_roots << <blocks, threads_per_block >> > (
			pos_x, pos_y, pos_z,
			prev_pos_x, prev_pos_y, prev_pos_z,
			root_dirs,
			num_strands,
			particles_per_strand,
			sphere_cx, sphere_cy, sphere_cz,
			sphere_radius,
			qx, qy, qz, qw);
	}

	void launch_solve_constraints_xpbd(
		float* pos_x,
		float* pos_y,
		float* pos_z,
		const float* inv_mass,
		const float* rest_lengths,
		float* lambdas,
		int   num_strands,
		int   num_segments,
		int   particles_per_strand,
		float compliance,
		float sub_dt,
		int   threads_per_block)
	{
		int blocks = (num_strands + threads_per_block - 1) / threads_per_block;
		k_solve_constraints_xpbd << <blocks, threads_per_block >> > (
			pos_x, pos_y, pos_z,
			inv_mass, rest_lengths, lambdas,
			num_strands, num_segments, particles_per_strand,
			compliance, sub_dt);
	}

	void launch_init_rand_states(
		curandState* states,
		int          total_particles,
		unsigned long long seed,
		int          threads_per_block)
	{
		int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
		k_init_rand_states << <blocks, threads_per_block >> > (states, total_particles, seed);
		cudaDeviceSynchronize();
	}

	void launch_translate_with_perturbation(
		float* pos_x, 
		float* pos_y, 
		float* pos_z,
		float* prev_pos_x, 
		float* prev_pos_y, 
		float* prev_pos_z,
		const float* inv_mass,
		curandState* states,
		int   total_particles,
		int   particles_per_strand,
		float delta_x, 
		float delta_y, 
		float delta_z,
		float drag_speed,
		int   threads_per_block)
	{
		int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
		k_translate_with_perturbation << <blocks, threads_per_block >> > (
			pos_x, pos_y, pos_z,
			prev_pos_x, prev_pos_y, prev_pos_z,
			inv_mass, states,
			total_particles, particles_per_strand,
			delta_x, delta_y, delta_z,
			drag_speed);
	}

	void launch_pack_positions_aos(
		const float* pos_x,
		const float* pos_y,
		const float* pos_z,
		float* aos_buffer,
		int total_particles,
		int num_strands,
		int particles_per_strand,
		int threads_per_block)
	{
		int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
		k_pack_positions_aos << <blocks, threads_per_block >> > (
			pos_x, pos_y, pos_z, aos_buffer,
			total_particles, num_strands, particles_per_strand);
	}

	void launch_apply_drag_velocity(
		float* pos_x,
		float* pos_y,
		float* pos_z,
		const float* prev_pos_x,
		const float* prev_pos_y,
		const float* prev_pos_z,
		const float* inv_mass,
		int   total_particles,
		float vel_x,
		float vel_y,
		float vel_z,
		float scale,
		int   threads_per_block)
	{
		int blocks = (total_particles + threads_per_block - 1) / threads_per_block;
		k_apply_drag_velocity << <blocks, threads_per_block >> > (
			pos_x, pos_y, pos_z,
			prev_pos_x, prev_pos_y, prev_pos_z,
			inv_mass,
			total_particles,
			vel_x, vel_y, vel_z, scale);
	}


} // namespace Toop