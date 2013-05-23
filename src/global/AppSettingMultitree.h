#ifndef APPSETTINGMULTITREE_H_
#define APPSETTINGMULTITREE_H_

#include "AppSetting.h"

class AppSettingMultitree : public AppSetting {
public:
    AppSettingMultitree(void);
    virtual ~AppSettingMultitree(void);

	inline int getNumStripes(void){ return param_numStripes; };
	inline double getWaitUntilInform(void) { return param_waitUntilInform; };

	inline int getChunkSize(void) { return param_chunkSize; };
	inline int getPacketSizeVideoChunk(void) { return param_chunkSize + 8; }; // 4 for seq_num, 4 for stripe_num
	inline int getVideoStreamBitRate(void) { return param_videoStreamBitRate; };

    inline int getBufferMapSizeChunk(void) { return param_bufferMapSizeChunk; };
    inline double getIntervalNewChunk(void) { return param_intervalNewChunk; };

protected:
    void handleMessage(cMessage* msg);
    virtual void initialize(void);
    virtual void finish(void);

    int param_numStripes;
    int param_stripeSize;
    double param_waitUntilInform;

    int param_chunkSize;
    int param_videoStreamBitRate;

    int param_bufferMapSizeChunk;
    double param_intervalNewChunk;

    int param_bufferMapSize_seconds;
    int m_videoStreamChunkRate;
 
};

#endif /* APPSETTINGMULTITREE_H_ */
