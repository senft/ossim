package so.global.statistic;

simple MultitreeStatistic like IStreamingStatistic
{
	parameters:
		string gstatLog = default("..//results//gstatlog.txt");

		double interval_reportGlobal @unit(s) = default(1s);

		@statistic[hopCount](source="Signal_Chunk_Arrival"; record=vector;);
        @statistic[meanNumTrees](source="Signal_Mean_Num_Trees"; record=vector);
        @statistic[bwUtil](source="Signal_BW_Utilization"; record=vector);
        @statistic[packetloss](source="Signal_Packet_Loss"; record=vector);
        @statistic[conntime](source="Signal_Connection_Time"; record=vector);
        @statistic[retrys](source="Signal_Retrys"; record=vector);
        @statistic[meanOutDegree](source="Signal_Mean_Out_Degree"; record=vector);

        @statistic[messageCountCR](source="Signal_Message_Count_CR"; record=vector);
        @statistic[messageCountDR](source="Signal_Message_Count_DR"; record=vector);
        @statistic[messageCountCC](source="Signal_Message_Count_CC"; record=vector);
        @statistic[messageCountPNR](source="Signal_Message_Count_PNR"; record=vector);
        @statistic[messageCountSI](source="Signal_Message_Count_SI"; record=vector);
}
