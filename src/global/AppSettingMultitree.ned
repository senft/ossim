package so.global;

simple AppSettingMultitree extends AppSetting
{
    parameters:
        @class(AppSettingMultitree);

        int numStripes =default(4);
        int stripeSize @unit(byte) = default(1250B);
        int videoStreamBitRate  @unit(bps)  = default(100kbps);
        int chunkSize           @unit(byte) = default(1250B);
        int bufferMapSize       @unit(s)    = default(2s);

        int delaySuccessorInfo @unit(s) = default(10ms);
		int intervalReconnect @unit(s) = default(2s);
}
