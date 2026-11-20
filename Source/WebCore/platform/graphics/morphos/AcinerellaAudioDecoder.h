#pragma once

#include "config.h"

#if ENABLE(VIDEO)

#include "AcinerellaDecoder.h"
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
namespace Acinerella {

class AcinerellaAudioDecoder : public AcinerellaDecoder
{
	AcinerellaAudioDecoder(AcinerellaDecoderClient* client, RefPtr<AcinerellaPointer> acinerella, RefPtr<AcinerellaMuxedBuffer> buffer, int index, const ac_stream_info &info, bool isLive, bool isHLS);
public:
	static RefPtr<AcinerellaAudioDecoder> create(AcinerellaDecoderClient* client, RefPtr<AcinerellaPointer> acinerella, RefPtr<AcinerellaMuxedBuffer> buffer, int index, const ac_stream_info &info, bool isLive, bool isHLS)
	{
		return WTF::adoptRef(*new AcinerellaAudioDecoder(client, acinerella, buffer, index, info, isLive, isHLS));
	}

	int rate() const { return m_audioRate; }
	int channels() const { return m_audioChannels; }
	int bits() const { return m_audioBits; }
	
	bool isAudio() const override { return true; }
	bool isVideo() const override { return false; }
	bool isText() const override { return false; }
	
	double readAheadTime() const override { return m_isLive ? 4.0 : 2.0; }

	bool isReadyToPlay() const override;
	bool isPlaying() const override;
    bool isWarmedUp() const override;

	double position() const override;
	double bufferSize() const override { return m_bufferedSeconds; }
	void paint(GraphicsContext&, const FloatRect&) override { }

	void dumpStatus() override;
#if OS(AROS)
	static void soundFunc(void *ptr);
#endif

protected:
	void startPlaying() override;
	void stopPlaying() override;
	void doSetVolume(double volume) override;

	void onThreadShutdown() override;
	void onGetReadyToPlay() override;
	void onFrameDecoded(const AcinerellaDecodedFrame &frame) override;
	void flush() override;
	bool initializeAudio();
	void onCoolDown() override;
	void ahiCleanup();

#if OS(MORPHOS)
	static void soundFunc();
#endif
	void fillBuffer(int index);
	void ahiThreadEntryPoint();
	void stopPlayingQuick() override { m_playing = false; };

protected:
#if OS(APLUS)
	struct SampleBuffer { void *ahisi_Address = nullptr; };
	std::unique_ptr<AplusAudioOutput> m_output;
	SampleBuffer    m_ahiSample[2];
	Lock            m_pumpLock;
	Condition       m_pumpCondition;
	std::atomic<bool> m_resetRequested { false };
	std::atomic<double> m_volume { 1.0 };
	double          m_announcedPosition = -1.0;
	double          m_writtenEnd = -1.0;
	bool            m_chunkTimed = false;
	void aplusPump();
#else
	Library        *m_ahiBase = nullptr;
	MsgPort        *m_ahiPort = nullptr;
	AHIRequest     *m_ahiIO = nullptr;
	AHIAudioCtrl   *m_ahiControl = nullptr;
	AHISampleInfo   m_ahiSample[2];
#endif
	double          m_ahiSampleTimestamp[2];
	uint32_t        m_ahiSampleLength; // *2 for bytes
	uint32_t        m_ahiSampleBeingPlayed;
	uint32_t        m_ahiFrameOffset; //
#if !OS(APLUS)
	Task           *m_pumpTask = nullptr;
#endif
	RefPtr<Thread>  m_ahiThread;
	bool            m_ahiThreadShuttingDown = false;

	uint32_t        m_bufferedSamples = 0;
	volatile float  m_bufferedSeconds = 0.f;
	volatile bool   m_playing = false;
	double          m_position = 0.0;
	double          m_liveTimeCode = 0.0;
	bool            m_waitingToPlay = false;
	bool            m_didFlushBuffers = false;
	bool            m_didUnderrun = false;
	int             m_audioRate;
	int             m_audioChannels;
	int             m_audioBits;
};

}
}

#endif
