package so.tree.multitree;

simple MultitreePeer extends MultitreeBase
{
    parameters:
        @class(MultitreePeer);
        @display("i=block/app;bgb=250,410");

        // Timer
        volatile double startTime @unit(s);

        volatile double intervalReconnect @unit(s);
}
