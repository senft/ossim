package so.simulations;

import inet.networklayer.autorouting.FlatNetworkConfigurator;
import inet.nodes.inet.Router;
import so.global.ActivePeerTable;
//import so.global.AppSettingDonet;
import so.global.AppSettingMultitree;
import so.global.churn.IChurnModel;
import so.tree.multitree.MultitreePeerNode;
import so.tree.multitree.MultitreeSourceNode;

import so.global.statistic.IStreamingStatistic;


network Multitree_Network extends BasicNetwork
{
    parameters:
        int nNodePerRouter;
        string churnModel = default("UniformChurn");
        string statisticModule = default("MultitreeStatistic");
        @display("bgb=472,295");

    types:
        channel Channel_9M_up extends ned.DatarateChannel
        {
            datarate = 9Mbps;
        }
        channel Channel_9M_down extends ned.DatarateChannel
        {
            datarate = 40Mbps;
        }

    submodules:
        sourceNode: MultitreeSourceNode {
            @display("p=395,51;i=device/server");
        }
        peerNode[nNodePerRouter]: MultitreePeerNode;
        router: Router {
            @display("p=306,150");
        }
        flatNetworkConfigurator: FlatNetworkConfigurator {
            @display("p=77,29");
            networkAddress = "192.168.0.0";
            netmask = "255.255.0.0";
        }
        activePeerTable: ActivePeerTable {
            @display("p=77,96");
        }
        appSetting: AppSettingMultitree {
            @display("p=77,254;i=block/wheelbarrow");
        }
        //globalStatistic: <statisticModule> like IStreamingStatistic {
        //    @display("p=77,195;i=device/disk");
        //}
        churnModerator: <churnModel> like IChurnModel {
            @display("p=77,171");
        }

    connections:
        // -- Connect source
        sourceNode.pppg$o++ --> Channel_9M_up --> router.pppg$i++;
        sourceNode.pppg$i++ <-- Channel_9M_down <-- router.pppg$o++;

        // -- Connect peers
        for i=0..nNodePerRouter-1 {
            peerNode[i].pppg++ <--> ethernetline <--> router.pppg++;
        }
}
