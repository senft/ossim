package so.simulations;

import so.global.ActivePeerTable;
import so.global.churn.IChurnModel;
import so.global.statistic.IDiscoveryStatistic;
import so.global.statistic.ScampStatistic;

import so.tree.mtreebone.mTreebonePeer;
import so.tree.mtreebone.mTreeboneSource;

import inet.nodes.inet.Router;
import inet.nodes.ethernet.EtherSwitch;
import inet.networklayer.autorouting.FlatNetworkConfigurator;

network mTreeboneNetwork extends BasicNetwork {
	parameters:
        int nNodePerRouter = default(1);
        @display("bgb=472,295");
        string churnModel = default("UniformChurn");
        string statisticModule = default("ScampStatistic");
	
	submodules:
        mTreeboneSource: mTreeboneSource{
            @display("p=395,51");
        }

        mTreebonePeer[nNodePerRouter]: mTreebonePeer;

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

        churnModerator: <churnModel> like IChurnModel {
            @display("p=77,171");
        }

        globalStatistic: <statisticModule> like IDiscoveryStatistic {
            @display("p=77,248");
        }
	
	connections:
        // -- Connect Source
        //mTreeboneSource.pppg$o++ --> Channel0_up --> router.pppg$i++;
        //mTreeboneSource.pppg$i++ <-- Channel0_down <-- router.pppg$o++;

        // -- Connect Peers
        //for i=0..nNodePerRouter-1 {
            //mTreebonePeer[i].pppg++ <--> ethernetline <--> router.pppg++;
        //}
}
