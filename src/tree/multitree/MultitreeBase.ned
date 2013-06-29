package so.tree.multitree;

import so.common.CommBase;

simple MultitreeBase extends CommBase
{
    parameters:
        @class(MultitreeBase);
        @display("i=block/app;bgb=250,410");

		double bwCapacity = default(1);

		double delayOptimization @unit(s) = default(1s);
		bool optimize = default(true);
		bool sendMissingChunks = default(true);
}
