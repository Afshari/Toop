#pragma once
#include <cuda_runtime.h>
#include <string>
#include <stdexcept>

namespace Toop {

    class CudaTimer
    {
    public:
        CudaTimer()
        {
            cudaEventCreate(&m_start);
            cudaEventCreate(&m_stop);
        }

        ~CudaTimer()
        {
            cudaEventDestroy(m_start);
            cudaEventDestroy(m_stop);
        }

        // non-copyable
        CudaTimer(const CudaTimer&) = delete;
        CudaTimer& operator=(const CudaTimer&) = delete;

        void Start()
        {
            cudaEventRecord(m_start);
        }

        void Stop()
        {
            cudaEventRecord(m_stop);
            cudaEventSynchronize(m_stop);
        }

        float ElapsedMs() const
        {
            float ms = 0.0f;
            cudaEventElapsedTime(&ms, m_start, m_stop);
            return ms;
        }

    private:
        cudaEvent_t m_start;
        cudaEvent_t m_stop;
    };

} // namespace Toop