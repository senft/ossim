package so.simulations;

import inet.networklayer.autorouting.FlatNetworkConfigurator;
import inet.nodes.inet.Router;
import so.global.ActivePeerTable;
import so.global.AppSettingMultitree;
import so.global.churn.IChurnModel;
import so.tree.multitree.MultitreePeerNode;
import so.tree.multitree.MultitreeSourceNode;

import so.global.statistic.IStreamingStatistic;


network Multitree_Network extends BasicNetwork
{
    parameters:
        string churnModel = default("BoundedChurn");
        string statisticModule = default("MultitreeStatistic");
        @display("bgb=472,324");

        int nNodePerRouter;

    types:
		channel Source_up extends ned.DatarateChannel
        {
            datarate = 1000kbps;
        }
        channel Source_down extends ned.DatarateChannel
        {
            datarate = 10Mbps;
        }

		channel DSL2k_up extends ned.DatarateChannel
        {
            datarate = 256kbps;
        }
        channel DSL2k_down extends ned.DatarateChannel
        {
            datarate = 2048kbps;
        }

		channel DSL6k_up extends ned.DatarateChannel
        {
            datarate = 576kbps;
        }
        channel DSL6k_down extends ned.DatarateChannel
        {
            datarate = 6016kbps;
        }

		channel DSL16k_up extends ned.DatarateChannel
        {
            datarate = 1024kbps;
        }
        channel DSL16k_down extends ned.DatarateChannel
        {
            datarate = 16000kbps;
        }

		channel Channel_50M_up extends ned.DatarateChannel
        {
            datarate = 50Mbps;
        }
        channel Channel_50M_down extends ned.DatarateChannel
        {
            datarate = 50Mbps;
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
        sourceNode.pppg$o++ --> Channel_50M_up --> router.pppg$i++;
        sourceNode.pppg$i++ <-- Channel_50M_down <-- router.pppg$o++;

        // -- Connect peers
		for i=0..(nNodePerRouter-1) {
            peerNode[i].pppg$o++ --> Channel_50M_up --> router.pppg$i++;
            peerNode[i].pppg$i++ <-- Channel_50M_down <-- router.pppg$o++;
        }
}
