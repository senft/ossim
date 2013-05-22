package so.global.statistic;

simple MultitreeStatistic like IStreamingStatistic
{
	parameters:

		string gstatLog = default("..//results//gstatlog.txt");

		@statistic[hopCount](source="Signal_Chunk_Arrival"; record=histogram,vector;);
}
