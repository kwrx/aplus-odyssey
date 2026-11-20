#include "config.h"

#include "FFTFrame.h"

#include "VectorMath.h"
#include <wtf/FastMalloc.h>
#include <wtf/StdLibExtras.h>

#if OS(APLUS)
extern "C" {
#include <libavutil/mem.h>
#include <libavutil/tx.h>
}
#endif

namespace {

const int kMinFFTPow2Size = 2;
const int kMaxFFTPow2Size = 24;

size_t unpackedFFTDataSize(unsigned fftSize)
{
    return fftSize / 2 + 1;
}

} // anonymous namespace

namespace WebCore {

// Normal constructor: allocates for a given fftSize.
FFTFrame::FFTFrame(unsigned fftSize)
    : m_FFTSize(fftSize)
    , m_log2FFTSize(static_cast<unsigned>(log2(fftSize)))
    , m_realData(unpackedFFTDataSize(m_FFTSize))
    , m_imagData(unpackedFFTDataSize(m_FFTSize))
{
#if OS(APLUS)
    createTransforms();
#endif
}

// Creates a blank/empty frame (interpolate() must later be called).
FFTFrame::FFTFrame()
    : m_FFTSize(0)
    , m_log2FFTSize(0)
{
}

// Copy constructor.
FFTFrame::FFTFrame(const FFTFrame& frame)
    : m_FFTSize(frame.m_FFTSize)
    , m_log2FFTSize(frame.m_log2FFTSize)
    , m_realData(unpackedFFTDataSize(frame.m_FFTSize))
    , m_imagData(unpackedFFTDataSize(frame.m_FFTSize))
{
#if OS(APLUS)
    createTransforms();
    memcpy(realData().data(), frame.realData().data(), sizeof(float) * realData().size());
    memcpy(imagData().data(), frame.imagData().data(), sizeof(float) * imagData().size());
#endif
}

void FFTFrame::initialize()
{
}

FFTFrame::~FFTFrame()
{
#if OS(APLUS)
    destroyTransforms();
#endif
}

#if OS(APLUS)
void FFTFrame::createTransforms()
{
    if (!m_FFTSize) {
        return;
    }

    float forwardScale = 1.0f;
    float inverseScale = 1.0f / m_FFTSize;

    if (av_tx_init(&m_forward, reinterpret_cast<av_tx_fn*>(&m_forwardFn), AV_TX_FLOAT_RDFT, 0, m_FFTSize, &forwardScale, 0) < 0) {
        m_forward = nullptr;
    }
    if (av_tx_init(&m_inverse, reinterpret_cast<av_tx_fn*>(&m_inverseFn), AV_TX_FLOAT_RDFT, 1, m_FFTSize, &inverseScale, 0) < 0) {
        m_inverse = nullptr;
    }

    m_timeBuffer = static_cast<float*>(av_malloc(sizeof(float) * (m_FFTSize + 2)));
    m_complexBuffer = static_cast<float*>(av_malloc(sizeof(float) * 2 * (unpackedFFTDataSize(m_FFTSize) + 1)));
}

void FFTFrame::destroyTransforms()
{
    av_tx_uninit(&m_forward);
    av_tx_uninit(&m_inverse);
    av_freep(&m_timeBuffer);
    av_freep(&m_complexBuffer);
}

void FFTFrame::doFFT(const float* data)
{
    if (!m_forward || !m_timeBuffer || !m_complexBuffer) {
        return;
    }

    memcpy(m_timeBuffer, data, sizeof(float) * m_FFTSize);
    m_forwardFn(m_forward, m_complexBuffer, m_timeBuffer, sizeof(float));

    float* realData = m_realData.data();
    float* imagData = m_imagData.data();
    for (size_t i = 0; i < unpackedFFTDataSize(m_FFTSize); ++i) {
        realData[i] = m_complexBuffer[2 * i];
        imagData[i] = m_complexBuffer[2 * i + 1];
    }
}

void FFTFrame::doInverseFFT(float* data)
{
    if (!m_inverse || !m_timeBuffer || !m_complexBuffer) {
        return;
    }

    const float* realData = m_realData.data();
    const float* imagData = m_imagData.data();
    for (size_t i = 0; i < unpackedFFTDataSize(m_FFTSize); ++i) {
        m_complexBuffer[2 * i] = realData[i];
        m_complexBuffer[2 * i + 1] = imagData[i];
    }

    m_inverseFn(m_inverse, m_timeBuffer, m_complexBuffer, 2 * sizeof(float));
    memcpy(data, m_timeBuffer, sizeof(float) * m_FFTSize);
}
#else
void FFTFrame::doFFT(const float* data)
{
}

void FFTFrame::doInverseFFT(float* data)
{
}
#endif

int FFTFrame::minFFTSize()
{
    return 1 << kMinFFTPow2Size;
}

int FFTFrame::maxFFTSize()
{
    return 1 << kMaxFFTPow2Size;
}

} // namespace WebCore
