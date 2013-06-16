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
        @display("bgb=472,324");

    types:
        channel Channel_9M_up extends ned.DatarateChannel
        {
            datarate = 9Mbps;
        }
        channel Channel_9M_down extends ned.DatarateChannel
        {
            datarate = 40Mbps;
        }

		channel DSL1k_up extends ned.DatarateChannel
        {
            datarate = 128kbps;
        }
        channel DSL1k_down extends ned.DatarateChannel
        {
            datarate = 1024kbps;
        }

		channel DSL6k_up extends ned.DatarateChannel
        {
            datarate = 576kbps;
        }
        channel DSL6k_down extends ned.DatarateChannel
        {
            datarate = 6016kbps;
        }

		channel channel_100mbit_up extends ned.DatarateChannel
        {
            datarate = 5Mbps;
        }
        channel channel_100mbit_down extends ned.DatarateChannel
        {
            datarate = 100Mbps;
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
            @display("p=77,281;i=block/wheelbarrow");
        }
        globalStatistic: <statisticModule> like IStreamingStatistic {
            @display("p=77,161;i=device/disk");
        }
        churnModerator: <churnModel> like IChurnModel {
            @display("p=77,220");
        }

    connections:
        // -- Connect source
        sourceNode.pppg$o++ --> DSL6k_up --> router.pppg$i++;
        sourceNode.pppg$i++ <-- DSL6k_down <-- router.pppg$o++;

        // -- Connect peers
        for i=0..nNodePerRouter-1 {
			peerNode[i].pppg$o++ --> DSL1k_up --> router.pppg$i++;
			peerNode[i].pppg$i++ <-- channel_100mbit_down <-- router.pppg$o++;
        }
}
