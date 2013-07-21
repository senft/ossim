package so.global.statistic;

simple MultitreeStatistic like IStreamingStatistic
{
	parameters:
		string gstatLog = default("..//results//gstatlog.txt");

		double interval_reportGlobal @unit(s) = default(1s);

        @statistic[bwUtil](source="Signal_BW_Utilization"; record=vector, mean);
        @statistic[packetloss](source="Signal_Packet_Loss"; record=vector, last);
        @statistic[conntime](source="Signal_Connection_Time"; record=vector, mean);
        @statistic[retrys](source="Signal_Retrys"; record=vector);

        @statistic[meanOutDegree](source="Signal_Mean_Out_Degree"; record=vector, mean);
		@statistic[hopCount](source="Signal_Chunk_Arrival"; record=vector, mean;);
        @statistic[meanNumTrees](source="Signal_Mean_Num_Trees"; record=vector, mean);

		@statistic[forwardingInOne](source="Signal_Forwarding_In_One"; record=vector, last;);
        @statistic[forwardingInMoreThanOne](source="Signal_Forwarding_In_More_thanOne"; record=vector, last);

        @statistic[messageCountCR](source="Signal_Message_Count_CR"; record=vector, last);
        @statistic[messageCountDR](source="Signal_Message_Count_DR"; record=vector, last);
        @statistic[messageCountCC](source="Signal_Message_Count_CC"; record=vector, last);
        @statistic[messageCountPNR](source="Signal_Message_Count_PNR"; record=vector, last);
        @statistic[messageCountSI](source="Signal_Message_Count_SI"; record=vector, last);
}
