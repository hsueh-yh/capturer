#ifndef INTERFACES_H_
#define INTERFACES_H_

/**
 * This class is used for delivering raw ARGB frames to the library.
 * After calling initPublishing, library returns a pointer of object
 * confirming this interface to a caller. Caller should use this pointer
 * for delivering frames into the library.
 * @see NdnRtcLibrary::initPublishing
 */
class IExternalCapturer
{
public:
    /**
     * This method should be called in order to initiate frame delivery into
     * the library
     */
    virtual void capturingStarted() = 0;

    /**
     * This method should be called in order to stop frame delivery into the
     * library
     */
    virtual void capturingStopped() = 0;

    /**
     * Calling this methond results in sending new raw frame into library's
     * video processing pipe which eventually should result in publishing
     * of encoded frame in NDN.
     * However, not every frame will be published - some frames are dropped
     * by the encoder.
     * @param bgraFramData Frame data in ARGB format
     * @param frameSize Size of the frame data
     */
    virtual int incomingArgbFrame(const unsigned int width,
                                  const unsigned int height,
                                  unsigned char* argbFrameData,
                                  unsigned int frameSize) = 0;

    /**
     * Alternative method for delivering YUV frames (I420 or y420 or
     * Y'CbCr 8-bit 4:2:0) inside the library. Either of two "incoming"
     * calls should be used.
     */
    virtual int incomingI420Frame(const unsigned int width,
                                  const unsigned int height,
                                  const unsigned int strideY,
                                  const unsigned int strideU,
                                  const unsigned int strideV,
                                  const unsigned char* yBuffer,
                                  const unsigned char* uBuffer,
                                  const unsigned char* vBuffer) = 0;
};

#endif //INTERFACES_H_
