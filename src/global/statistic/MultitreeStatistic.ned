package so.global.statistic;

simple MultitreeStatistic like IStreamingStatistic
{
	parameters:
		string gstatLog = default("..//results//gstatlog.txt");

		double interval_reportGlobal @unit(s) = default(1s);

		@statistic[hopCount](source="Signal_Chunk_Arrival"; record=histogram,vector;);
        @statistic[meanNumTrees](source="Signal_Mean_Num_Trees"; record=vector);
        @statistic[bwUtil](source="Signal_BW_Utilization"; record=vector);
        @statistic[packetloss](source="Signal_Packet_Loss"; record=vector);
        @statistic[conntime](source="Signal_Connection_Time"; record=vector);
        @statistic[retrys](source="Signal_Retrys"; record=vector);
}
