#pragma once
#include "AudioBus.h"
#include <wtf/MessageQueue.h>
#include <wtf/Threading.h>
#include <wtf/threads/BinarySemaphore.h>
#if OS(APLUS)
#include "AplusAudioOutput.h"
#include <atomic>
#include <wtf/Condition.h>
#include <wtf/Lock.h>
#endif
#if !OS(APLUS)
#include <devices/ahi.h>
#include <exec/libraries.h>
#endif

namespace WebCore {

class AudioDestinationRenderer
{
public:
	virtual void render(int16_t *samplesStereo16s, size_t count) = 0;
};

class AudioDestinationOutputMorphOS
{
public:
	AudioDestinationOutputMorphOS(AudioDestinationRenderer &parent);
	~AudioDestinationOutputMorphOS();

	bool start();
	void stop();

#if OS(AROS)
	static void soundFunc(void *ptr);
#endif

protected:
	bool ahiInit();
	void ahiCleanup();
	void ahiThreadEntryPoint();

#if OS(MORPHOS)
	static void soundFunc();
#endif

protected:
	AudioDestinationRenderer& m_renderer;

#if OS(APLUS)
	std::unique_ptr<AplusAudioOutput> m_output;
	int16_t        *m_buffer[2] = { nullptr, nullptr };
	uint32_t        m_ahiSampleLength = 0;
	uint32_t        m_ahiSampleBeingPlayed = 0;
	Lock            m_pumpLock;
	Condition       m_pumpCondition;
	std::atomic<bool> m_running { false };
	RefPtr<Thread>  m_ahiThread;
	std::atomic<bool> m_ahiThreadShuttingDown { false };
#else
	Library        *m_ahiBase = nullptr;
	MsgPort        *m_ahiPort = nullptr;
	AHIRequest     *m_ahiIO = nullptr;
	AHIAudioCtrl   *m_ahiControl = nullptr;
	AHISampleInfo   m_ahiSample[2];
	uint32_t        m_ahiSampleLength;
	uint32_t        m_ahiSampleBeingPlayed;

	Task           *m_pumpTask = nullptr;
	RefPtr<Thread>  m_ahiThread;
	bool            m_ahiThreadShuttingDown = false;
#endif
};

} // namespace
