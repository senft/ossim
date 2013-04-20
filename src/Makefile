#
# OMNeT++/OMNEST Makefile for so
#
# This file was generated with the command:
#  opp_makemake -f -O out --deep -o so -linet -I../../inet/src/networklayer/ipv4 -I../../inet/src/networklayer/common -I../../inet/src/networklayer/rsvp_te -I../../inet/src/networklayer/icmpv6 -I../../inet/src/transport/tcp -I../../inet/src/base -I../../inet/src/networklayer/mpls -I../../inet/src/networklayer/ted -I../../inet/src/util/headerserializers -I../../inet/src/networklayer/contract -I../../inet/src/util -I../../inet/src/transport/contract -I../../inet/src/networklayer/ipv6 -I../../inet/src/transport/sctp -I../../inet/src/linklayer/mfcore -I../../inet/src/world -I../../inet/src/applications/pingapp -I../../inet/src/linklayer/contract -I../../inet/src/networklayer/arp -I../../inet/src/networklayer/ldp -I../../inet/src/transport/udp -I../../inet/src/applications/udpapp -I. -I../../omnetpp-4.2.2/include -L../../inet/src -L/usr/lib
#

# Name of target to be created (-o option)
TARGET = so$(EXE_SUFFIX)

# User interface (uncomment one) (-u option)
USERIF_LIBS = $(ALL_ENV_LIBS) # that is, $(TKENV_LIBS) $(CMDENV_LIBS)
#USERIF_LIBS = $(CMDENV_LIBS)
#USERIF_LIBS = $(TKENV_LIBS)

# C++ include paths (with -I)
INCLUDE_PATH = \
    -I../../inet/src/networklayer/ipv4 \
    -I../../inet/src/networklayer/common \
    -I../../inet/src/networklayer/rsvp_te \
    -I../../inet/src/networklayer/icmpv6 \
    -I../../inet/src/transport/tcp \
    -I../../inet/src/base \
    -I../../inet/src/networklayer/mpls \
    -I../../inet/src/networklayer/ted \
    -I../../inet/src/util/headerserializers \
    -I../../inet/src/networklayer/contract \
    -I../../inet/src/util \
    -I../../inet/src/transport/contract \
    -I../../inet/src/networklayer/ipv6 \
    -I../../inet/src/transport/sctp \
    -I../../inet/src/linklayer/mfcore \
    -I../../inet/src/world \
    -I../../inet/src/applications/pingapp \
    -I../../inet/src/linklayer/contract \
    -I../../inet/src/networklayer/arp \
    -I../../inet/src/networklayer/ldp \
    -I../../inet/src/transport/udp \
    -I../../inet/src/applications/udpapp \
    -I. \
    -I../../omnetpp-4.2.2/include \
    -I. \
    -Icommon \
    -Iglobal \
    -Iglobal/churn \
    -Iglobal/logger \
    -Iglobal/observer \
    -Iglobal/statistic \
    -Imembership \
    -Imembership/dummy \
    -Imembership/newscast \
    -Imembership/oracle \
    -Imembership/scamp \
    -Imesh \
    -Imesh/coolstreaming \
    -Imesh/donet \
    -Ipayload \
    -Itree

# Additional object and library files to link with
EXTRA_OBJS =

# Additional libraries (-L, -l options)
LIBS = -L../../inet/src -L/usr/lib  -linet
LIBS += -Wl,-rpath,`abspath ../../inet/src` -Wl,-rpath,`abspath /usr/lib`

# Output directory
PROJECT_OUTPUT_DIR = ../out
PROJECTRELATIVE_PATH = /src
O = $(PROJECT_OUTPUT_DIR)/$(CONFIGNAME)/$(PROJECTRELATIVE_PATH)

# Object files for local .cc and .msg files
OBJS = \
    $O/common/CommBase.o \
    $O/common/Dispatcher.o \
    $O/global/ActivePeerInfo.o \
    $O/global/ActivePeerTable.o \
    $O/global/AppSetting.o \
    $O/global/AppSettingDonet.o \
    $O/global/churn/ExponentialChurnModel.o \
    $O/global/churn/PoissonChurnModel.o \
    $O/global/churn/StaticChurn.o \
    $O/global/churn/UniformChurn.o \
    $O/global/churn/VelosoChurnModel.o \
    $O/global/logger/Logger.o \
    $O/global/observer/MeshOverlayObserver.o \
    $O/global/statistic/GlobalStatistic.o \
    $O/global/statistic/Statistic.o \
    $O/membership/Contact.o \
    $O/membership/MCache.o \
    $O/membership/MembershipBase.o \
    $O/membership/MessageLogger.o \
    $O/membership/dummy/DummyMembership.o \
    $O/membership/newscast/GossipProtocol.o \
    $O/membership/newscast/GossipProtocolWithUserData.o \
    $O/membership/newscast/GossipUserData.o \
    $O/membership/newscast/NewscastBase.o \
    $O/membership/newscast/NewscastCache.o \
    $O/membership/newscast/NewscastCacheEntry.o \
    $O/membership/newscast/NewscastPeer.o \
    $O/membership/newscast/NewscastSource.o \
    $O/membership/newscast/NewscastStatistic.o \
    $O/membership/oracle/OracleMembership.o \
    $O/membership/scamp/ScampBase.o \
    $O/membership/scamp/ScampPeer.o \
    $O/membership/scamp/ScampSource.o \
    $O/mesh/BufferMap.o \
    $O/mesh/NeighborInfo.o \
    $O/mesh/PartnerList.o \
    $O/mesh/coolstreaming/CoolstreamingBase.o \
    $O/mesh/coolstreaming/CoolstreamingPartner.o \
    $O/mesh/coolstreaming/CoolstreamingPeer.o \
    $O/mesh/coolstreaming/CoolstreamingSource.o \
    $O/mesh/donet/DonetBase.o \
    $O/mesh/donet/DonetPeer.o \
    $O/mesh/donet/DonetScheduling.o \
    $O/mesh/donet/DonetSource.o \
    $O/mesh/donet/RandomScheduling.o \
    $O/payload/ChunkGenerator.o \
    $O/payload/Forwarder.o \
    $O/payload/Player.o \
    $O/payload/PlayerSimpleSkip.o \
    $O/payload/VideoBuffer.o \
    $O/payload/VideoBufferListener.o \
    $O/common/DpControlInfo_m.o \
    $O/common/PeerStreamingPacket_m.o \
    $O/membership/GossipMembershipPacket_m.o \
    $O/membership/newscast/NewscastPacket_m.o \
    $O/mesh/MeshPeerStreamingPacket_m.o \
    $O/mesh/coolstreaming/CoolstreamingPacket_m.o \
    $O/payload/VideoChunkPacket_m.o \
    $O/tree/TreePeerStreamingPacket_m.o

# Message files
MSGFILES = \
    common/DpControlInfo.msg \
    common/PeerStreamingPacket.msg \
    membership/GossipMembershipPacket.msg \
    membership/newscast/NewscastPacket.msg \
    mesh/MeshPeerStreamingPacket.msg \
    mesh/coolstreaming/CoolstreamingPacket.msg \
    payload/VideoChunkPacket.msg \
    tree/TreePeerStreamingPacket.msg

#------------------------------------------------------------------------------

# Pull in OMNeT++ configuration (Makefile.inc or configuser.vc)

ifneq ("$(OMNETPP_CONFIGFILE)","")
CONFIGFILE = $(OMNETPP_CONFIGFILE)
else
ifneq ("$(OMNETPP_ROOT)","")
CONFIGFILE = $(OMNETPP_ROOT)/Makefile.inc
else
CONFIGFILE = $(shell opp_configfilepath)
endif
endif

ifeq ("$(wildcard $(CONFIGFILE))","")
$(error Config file '$(CONFIGFILE)' does not exist -- add the OMNeT++ bin directory to the path so that opp_configfilepath can be found, or set the OMNETPP_CONFIGFILE variable to point to Makefile.inc)
endif

include $(CONFIGFILE)

# Simulation kernel and user interface libraries
OMNETPP_LIB_SUBDIR = $(OMNETPP_LIB_DIR)/$(TOOLCHAIN_NAME)
OMNETPP_LIBS = -L"$(OMNETPP_LIB_SUBDIR)" -L"$(OMNETPP_LIB_DIR)" -loppmain$D $(USERIF_LIBS) $(KERNEL_LIBS) $(SYS_LIBS)

COPTS = $(CFLAGS)  $(INCLUDE_PATH) -I$(OMNETPP_INCL_DIR)
MSGCOPTS = $(INCLUDE_PATH)

# we want to recompile everything if COPTS changes,
# so we store COPTS into $COPTS_FILE and have object
# files depend on it (except when "make depend" was called)
COPTS_FILE = $O/.last-copts
ifneq ($(MAKECMDGOALS),depend)
ifneq ("$(COPTS)","$(shell cat $(COPTS_FILE) 2>/dev/null || echo '')")
$(shell $(MKPATH) "$O" && echo "$(COPTS)" >$(COPTS_FILE))
endif
endif

#------------------------------------------------------------------------------
# User-supplied makefile fragment(s)
# >>>
# <<<
#------------------------------------------------------------------------------

# Main target
all: $O/$(TARGET)
	$(LN) $O/$(TARGET) .

$O/$(TARGET): $(OBJS)  $(wildcard $(EXTRA_OBJS)) Makefile
	@$(MKPATH) $O
	$(CXX) $(LDFLAGS) -o $O/$(TARGET)  $(OBJS) $(EXTRA_OBJS) $(AS_NEEDED_OFF) $(WHOLE_ARCHIVE_ON) $(LIBS) $(WHOLE_ARCHIVE_OFF) $(OMNETPP_LIBS)

.PHONY: all clean cleanall depend msgheaders

.SUFFIXES: .cc

$O/%.o: %.cc $(COPTS_FILE)
	@$(MKPATH) $(dir $@)
	$(CXX) -c $(COPTS) -o $@ $<

%_m.cc %_m.h: %.msg
	$(MSGC) -s _m.cc $(MSGCOPTS) $?

msgheaders: $(MSGFILES:.msg=_m.h)

clean:
	-rm -rf $O
	-rm -f so so.exe libso.so libso.a libso.dll libso.dylib
	-rm -f ./*_m.cc ./*_m.h
	-rm -f common/*_m.cc common/*_m.h
	-rm -f global/*_m.cc global/*_m.h
	-rm -f global/churn/*_m.cc global/churn/*_m.h
	-rm -f global/logger/*_m.cc global/logger/*_m.h
	-rm -f global/observer/*_m.cc global/observer/*_m.h
	-rm -f global/statistic/*_m.cc global/statistic/*_m.h
	-rm -f membership/*_m.cc membership/*_m.h
	-rm -f membership/dummy/*_m.cc membership/dummy/*_m.h
	-rm -f membership/newscast/*_m.cc membership/newscast/*_m.h
	-rm -f membership/oracle/*_m.cc membership/oracle/*_m.h
	-rm -f membership/scamp/*_m.cc membership/scamp/*_m.h
	-rm -f mesh/*_m.cc mesh/*_m.h
	-rm -f mesh/coolstreaming/*_m.cc mesh/coolstreaming/*_m.h
	-rm -f mesh/donet/*_m.cc mesh/donet/*_m.h
	-rm -f payload/*_m.cc payload/*_m.h
	-rm -f tree/*_m.cc tree/*_m.h

cleanall: clean
	-rm -rf $(PROJECT_OUTPUT_DIR)

depend:
	$(MAKEDEPEND) $(INCLUDE_PATH) -f Makefile -P\$$O/ -- $(MSG_CC_FILES)  ./*.cc common/*.cc global/*.cc global/churn/*.cc global/logger/*.cc global/observer/*.cc global/statistic/*.cc membership/*.cc membership/dummy/*.cc membership/newscast/*.cc membership/oracle/*.cc membership/scamp/*.cc mesh/*.cc mesh/coolstreaming/*.cc mesh/donet/*.cc payload/*.cc tree/*.cc

# DO NOT DELETE THIS LINE -- make depend depends on it.
$O/common/CommBase.o: common/CommBase.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  common/AppCommon.h \
  ../../inet/src/networklayer/common/InterfaceTableAccess.h \
  common/DpControlInfo_m.h \
  global/churn/IChurnGenerator.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  global/statistic/GlobalStatistic.h \
  ../../inet/src/networklayer/ipv4/IPv4InterfaceData.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/base/INETDefs.h \
  common/CommBase.h \
  global/ActivePeerTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  global/ActivePeerInfo.h
$O/common/Dispatcher.o: common/Dispatcher.cc \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  common/Dispatcher.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/DpControlInfo_m.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/base/INETDefs.h \
  ../../inet/src/transport/contract/UDPControlInfo_m.h
$O/common/DpControlInfo_m.o: common/DpControlInfo_m.cc \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/DpControlInfo_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/base/INETDefs.h
$O/common/PeerStreamingPacket_m.o: common/PeerStreamingPacket_m.cc \
  common/PeerStreamingPacket_m.h
$O/global/ActivePeerInfo.o: global/ActivePeerInfo.cc \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/base/INETDefs.h \
  global/ActivePeerInfo.h
$O/global/ActivePeerTable.o: global/ActivePeerTable.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  common/AppCommon.h \
  global/churn/IChurnGenerator.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  global/statistic/GlobalStatistic.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/base/INETDefs.h \
  global/ActivePeerTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  global/ActivePeerInfo.h
$O/global/AppSetting.o: global/AppSetting.cc \
  global/AppSetting.h
$O/global/AppSettingDonet.o: global/AppSettingDonet.cc \
  global/AppSetting.h \
  global/AppSettingDonet.h
$O/global/churn/ExponentialChurnModel.o: global/churn/ExponentialChurnModel.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  global/churn/IChurnGenerator.h \
  global/churn/ExponentialChurnModel.h \
  ../../inet/src/base/INETDefs.h \
  ../../inet/src/base/NotificationBoard.h
$O/global/churn/PoissonChurnModel.o: global/churn/PoissonChurnModel.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  global/churn/IChurnGenerator.h \
  ../../inet/src/base/INETDefs.h \
  ../../inet/src/base/NotificationBoard.h \
  global/churn/PoissonChurnModel.h
$O/global/churn/StaticChurn.o: global/churn/StaticChurn.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  global/churn/StaticChurn.h \
  global/churn/IChurnGenerator.h \
  ../../inet/src/base/INETDefs.h \
  ../../inet/src/base/NotificationBoard.h
$O/global/churn/UniformChurn.o: global/churn/UniformChurn.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  global/churn/UniformChurn.h \
  ../../inet/src/base/ModuleAccess.h \
  global/churn/IChurnGenerator.h \
  ../../inet/src/base/INETDefs.h \
  ../../inet/src/base/NotificationBoard.h
$O/global/churn/VelosoChurnModel.o: global/churn/VelosoChurnModel.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  global/churn/IChurnGenerator.h \
  ../../inet/src/base/INETDefs.h \
  global/churn/VelosoChurnModel.h \
  ../../inet/src/base/NotificationBoard.h
$O/global/logger/Logger.o: global/logger/Logger.cc \
  common/AppCommon.h \
  global/logger/Logger.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/base/INETDefs.h
$O/global/observer/MeshOverlayObserver.o: global/observer/MeshOverlayObserver.cc \
  common/AppCommon.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  global/observer/MeshOverlayObserver.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/base/INETDefs.h
$O/global/statistic/GlobalStatistic.o: global/statistic/GlobalStatistic.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  common/AppCommon.h \
  global/churn/IChurnGenerator.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  global/statistic/GlobalStatistic.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/base/INETDefs.h \
  global/ActivePeerTable.h \
  ../../inet/src/base/NotificationBoard.h
$O/global/statistic/Statistic.o: global/statistic/Statistic.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  global/statistic/Statistic.h \
  global/churn/IChurnGenerator.h \
  ../../inet/src/base/INETDefs.h \
  ../../inet/src/base/NotificationBoard.h
$O/membership/Contact.o: membership/Contact.cc \
  membership/Contact.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/base/INETDefs.h \
  ../../inet/src/transport/contract/UDPControlInfo_m.h \
  ../../inet/src/transport/udp/UDP.h
$O/membership/GossipMembershipPacket_m.o: membership/GossipMembershipPacket_m.cc \
  membership/GossipMembershipPacket_m.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/base/INETDefs.h
$O/membership/MCache.o: membership/MCache.cc \
  membership/MCache.h
$O/membership/MembershipBase.o: membership/MembershipBase.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  common/AppCommon.h \
  global/churn/IChurnGenerator.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  global/statistic/GlobalStatistic.h \
  membership/MembershipBase.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/base/INETDefs.h \
  common/CommBase.h \
  global/ActivePeerTable.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  global/ActivePeerInfo.h
$O/membership/MessageLogger.o: membership/MessageLogger.cc \
  membership/MessageLogger.h \
  common/AppCommon.h \
  membership/GossipMembershipPacket_m.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/base/INETDefs.h
$O/membership/dummy/DummyMembership.o: membership/dummy/DummyMembership.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  membership/dummy/DummyMembership.h \
  common/AppCommon.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  membership/MembershipBase.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/base/INETDefs.h \
  global/ActivePeerTable.h \
  common/CommBase.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  global/ActivePeerInfo.h
$O/membership/newscast/GossipProtocol.o: membership/newscast/GossipProtocol.cc \
  membership/newscast/GossipProtocol.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/base/INETDefs.h
$O/membership/newscast/GossipProtocolWithUserData.o: membership/newscast/GossipProtocolWithUserData.cc \
  membership/newscast/GossipProtocol.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  membership/newscast/GossipProtocolWithUserData.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/base/INETDefs.h \
  membership/newscast/GossipUserData.h
$O/membership/newscast/GossipUserData.o: membership/newscast/GossipUserData.cc \
  membership/newscast/GossipUserData.h
$O/membership/newscast/NewscastBase.o: membership/newscast/NewscastBase.cc \
  membership/newscast/NewscastCache.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  membership/newscast/NewscastAgentInterface.h \
  common/Dispatcher.h \
  membership/newscast/NewscastBase.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  membership/newscast/GossipProtocolWithUserData.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  membership/newscast/NewscastPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  global/ActivePeerTable.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  membership/newscast/GossipUserData.h \
  membership/newscast/NewscastCacheEntry.h \
  common/AppCommon.h \
  ../../inet/src/networklayer/common/InterfaceTableAccess.h \
  membership/newscast/NewscastStatistic.h \
  membership/newscast/GossipProtocol.h \
  common/DpControlInfo_m.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  ../../inet/src/networklayer/ipv4/IPv4InterfaceData.h \
  ../../inet/src/base/INETDefs.h \
  global/ActivePeerInfo.h
$O/membership/newscast/NewscastCache.o: membership/newscast/NewscastCache.cc \
  membership/newscast/NewscastCache.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  membership/newscast/GossipUserData.h \
  membership/newscast/NewscastCacheEntry.h \
  ../../inet/src/base/INETDefs.h
$O/membership/newscast/NewscastCacheEntry.o: membership/newscast/NewscastCacheEntry.cc \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  membership/newscast/GossipUserData.h \
  membership/newscast/NewscastCacheEntry.h \
  ../../inet/src/base/INETDefs.h
$O/membership/newscast/NewscastPacket_m.o: membership/newscast/NewscastPacket_m.cc \
  membership/newscast/NewscastCache.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  membership/newscast/NewscastPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  membership/newscast/GossipUserData.h \
  membership/newscast/NewscastCacheEntry.h \
  ../../inet/src/base/INETDefs.h
$O/membership/newscast/NewscastPeer.o: membership/newscast/NewscastPeer.cc \
  membership/newscast/NewscastCache.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  membership/newscast/NewscastAgentInterface.h \
  common/Dispatcher.h \
  membership/newscast/NewscastBase.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  membership/newscast/GossipProtocolWithUserData.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  global/ActivePeerTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  membership/newscast/GossipUserData.h \
  ../../inet/src/base/NotificationBoard.h \
  membership/newscast/NewscastCacheEntry.h \
  common/AppCommon.h \
  membership/newscast/NewscastStatistic.h \
  membership/newscast/GossipProtocol.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  ../../inet/src/base/INETDefs.h \
  global/ActivePeerInfo.h \
  membership/newscast/NewscastPeer.h
$O/membership/newscast/NewscastSource.o: membership/newscast/NewscastSource.cc \
  membership/newscast/NewscastCache.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  membership/newscast/NewscastAgentInterface.h \
  common/Dispatcher.h \
  membership/newscast/NewscastBase.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  membership/newscast/GossipProtocolWithUserData.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  global/ActivePeerTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  membership/newscast/GossipUserData.h \
  membership/newscast/NewscastSource.h \
  membership/newscast/NewscastCacheEntry.h \
  common/AppCommon.h \
  membership/newscast/NewscastStatistic.h \
  membership/newscast/GossipProtocol.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  ../../inet/src/base/INETDefs.h \
  global/ActivePeerInfo.h
$O/membership/newscast/NewscastStatistic.o: membership/newscast/NewscastStatistic.cc \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  membership/newscast/NewscastStatistic.h \
  ../../inet/src/base/INETDefs.h
$O/membership/oracle/OracleMembership.o: membership/oracle/OracleMembership.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  global/ActivePeerTable.h \
  common/CommBase.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  common/AppCommon.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  membership/MembershipBase.h \
  ../../inet/src/base/INETDefs.h \
  membership/oracle/OracleMembership.h \
  global/ActivePeerInfo.h
$O/membership/scamp/ScampBase.o: membership/scamp/ScampBase.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  common/Dispatcher.h \
  membership/MessageLogger.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  common/CommBase.h \
  global/ActivePeerTable.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  common/AppCommon.h \
  membership/scamp/ScampBase.h \
  ../../inet/src/networklayer/common/InterfaceTableAccess.h \
  membership/GossipMembershipPacket_m.h \
  membership/Contact.h \
  global/churn/IChurnGenerator.h \
  common/DpControlInfo_m.h \
  global/statistic/GlobalStatistic.h \
  ../../inet/src/networklayer/ipv4/IPv4InterfaceData.h \
  membership/MembershipBase.h \
  ../../inet/src/base/INETDefs.h \
  global/ActivePeerInfo.h
$O/membership/scamp/ScampPeer.o: membership/scamp/ScampPeer.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  common/Dispatcher.h \
  membership/MessageLogger.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/transport/udp/UDPPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  common/CommBase.h \
  global/ActivePeerTable.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  ../../inet/src/transport/udp/UDPPacket.h \
  membership/scamp/ScampBase.h \
  common/AppCommon.h \
  ../../inet/src/networklayer/common/InterfaceTableAccess.h \
  ../../inet/src/networklayer/contract/IPAddressResolver.h \
  membership/GossipMembershipPacket_m.h \
  membership/Contact.h \
  common/DpControlInfo_m.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  membership/scamp/ScampPeer.h \
  membership/MembershipBase.h \
  ../../inet/src/networklayer/ipv4/IPv4InterfaceData.h \
  ../../inet/src/base/INETDefs.h \
  ../../inet/src/applications/udpapp/UDPBasicApp.h \
  global/ActivePeerInfo.h
$O/membership/scamp/ScampSource.o: membership/scamp/ScampSource.cc \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  common/Dispatcher.h \
  membership/MessageLogger.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  global/ActivePeerTable.h \
  common/CommBase.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  membership/scamp/ScampBase.h \
  common/AppCommon.h \
  ../../inet/src/networklayer/common/InterfaceTableAccess.h \
  membership/Contact.h \
  membership/GossipMembershipPacket_m.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  membership/MembershipBase.h \
  ../../inet/src/networklayer/ipv4/IPv4InterfaceData.h \
  ../../inet/src/base/INETDefs.h \
  ../../inet/src/transport/contract/UDPControlInfo_m.h \
  membership/scamp/ScampSource.h \
  global/ActivePeerInfo.h
$O/mesh/BufferMap.o: mesh/BufferMap.cc \
  mesh/BufferMap.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  common/AppCommon.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h
$O/mesh/MeshPeerStreamingPacket_m.o: mesh/MeshPeerStreamingPacket_m.cc \
  common/PeerStreamingPacket_m.h \
  mesh/MeshPeerStreamingPacket_m.h
$O/mesh/NeighborInfo.o: mesh/NeighborInfo.cc \
  mesh/BufferMap.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  mesh/NeighborInfo.h \
  common/AppCommon.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h
$O/mesh/PartnerList.o: mesh/PartnerList.cc \
  mesh/BufferMap.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  global/AppSetting.h \
  mesh/NeighborInfo.h \
  mesh/PartnerList.h \
  common/AppCommon.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h
$O/mesh/coolstreaming/CoolstreamingBase.o: mesh/coolstreaming/CoolstreamingBase.cc \
  ../../inet/src/base/INotifiable.h \
  mesh/BufferMap.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  payload/VideoChunkPacket_m.h \
  common/Dispatcher.h \
  ../../inet/src/base/ModuleAccess.h \
  global/logger/Logger.h \
  payload/Forwarder.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  global/AppSetting.h \
  mesh/NeighborInfo.h \
  common/CommBase.h \
  global/ActivePeerTable.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  mesh/coolstreaming/CoolstreamingBase.h \
  ../../inet/src/base/NotificationBoard.h \
  mesh/PartnerList.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  mesh/coolstreaming/CoolstreamingPartner.h \
  membership/newscast/GossipProtocol.h \
  global/observer/MeshOverlayObserver.h \
  global/churn/IChurnGenerator.h \
  common/DpControlInfo_m.h \
  global/statistic/GlobalStatistic.h \
  mesh/coolstreaming/CoolstreamingPacket_m.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h \
  global/ActivePeerInfo.h
$O/mesh/coolstreaming/CoolstreamingPacket_m.o: mesh/coolstreaming/CoolstreamingPacket_m.cc \
  common/PeerStreamingPacket_m.h \
  mesh/coolstreaming/CoolstreamingPacket_m.h \
  mesh/MeshPeerStreamingPacket_m.h
$O/mesh/coolstreaming/CoolstreamingPartner.o: mesh/coolstreaming/CoolstreamingPartner.cc \
  payload/VideoChunkPacket_m.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  mesh/coolstreaming/CoolstreamingPartner.h \
  mesh/coolstreaming/CoolstreamingPacket_m.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h
$O/mesh/coolstreaming/CoolstreamingPeer.o: mesh/coolstreaming/CoolstreamingPeer.cc \
  mesh/BufferMap.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  common/Dispatcher.h \
  ../../inet/src/base/ModuleAccess.h \
  global/logger/Logger.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  payload/Forwarder.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  global/AppSetting.h \
  mesh/NeighborInfo.h \
  global/ActivePeerTable.h \
  common/CommBase.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  mesh/coolstreaming/CoolstreamingBase.h \
  ../../inet/src/base/NotificationBoard.h \
  mesh/coolstreaming/CoolstreamingPeer.h \
  mesh/PartnerList.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  mesh/coolstreaming/CoolstreamingPartner.h \
  membership/newscast/GossipProtocol.h \
  global/churn/IChurnGenerator.h \
  global/observer/MeshOverlayObserver.h \
  mesh/coolstreaming/CoolstreamingPacket_m.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h \
  global/ActivePeerInfo.h
$O/mesh/coolstreaming/CoolstreamingSource.o: mesh/coolstreaming/CoolstreamingSource.cc \
  mesh/BufferMap.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  common/Dispatcher.h \
  ../../inet/src/base/ModuleAccess.h \
  global/logger/Logger.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  payload/Forwarder.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  global/AppSetting.h \
  mesh/NeighborInfo.h \
  global/ActivePeerTable.h \
  common/CommBase.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  mesh/coolstreaming/CoolstreamingBase.h \
  ../../inet/src/base/NotificationBoard.h \
  mesh/PartnerList.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  mesh/coolstreaming/CoolstreamingPartner.h \
  membership/newscast/GossipProtocol.h \
  global/churn/IChurnGenerator.h \
  global/observer/MeshOverlayObserver.h \
  mesh/coolstreaming/CoolstreamingPacket_m.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  mesh/coolstreaming/CoolstreamingSource.h \
  global/AppSettingDonet.h \
  global/ActivePeerInfo.h
$O/mesh/donet/DonetBase.o: mesh/donet/DonetBase.cc \
  ../../inet/src/base/INotifiable.h \
  mesh/BufferMap.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  payload/VideoChunkPacket_m.h \
  common/Dispatcher.h \
  ../../inet/src/base/ModuleAccess.h \
  global/logger/Logger.h \
  payload/Forwarder.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  global/AppSetting.h \
  mesh/NeighborInfo.h \
  common/CommBase.h \
  global/ActivePeerTable.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  mesh/PartnerList.h \
  mesh/donet/DonetBase.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  global/churn/IChurnGenerator.h \
  global/observer/MeshOverlayObserver.h \
  common/DpControlInfo_m.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h \
  global/ActivePeerInfo.h
$O/mesh/donet/DonetPeer.o: mesh/donet/DonetPeer.cc \
  mesh/BufferMap.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  payload/PlayerBase.h \
  common/Dispatcher.h \
  ../../inet/src/base/ModuleAccess.h \
  global/logger/Logger.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  payload/Forwarder.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  global/AppSetting.h \
  mesh/NeighborInfo.h \
  global/ActivePeerTable.h \
  common/CommBase.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  mesh/donet/DonetBase.h \
  mesh/PartnerList.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  mesh/donet/DonetPeer.h \
  ../../inet/src/networklayer/common/InterfaceTableAccess.h \
  global/observer/MeshOverlayObserver.h \
  common/DpControlInfo_m.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/networklayer/ipv4/IPv4InterfaceData.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h \
  global/ActivePeerInfo.h
$O/mesh/donet/DonetScheduling.o: mesh/donet/DonetScheduling.cc \
  mesh/BufferMap.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  payload/PlayerBase.h \
  common/Dispatcher.h \
  ../../inet/src/base/ModuleAccess.h \
  global/logger/Logger.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  payload/Forwarder.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  global/AppSetting.h \
  mesh/NeighborInfo.h \
  global/ActivePeerTable.h \
  common/CommBase.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  mesh/donet/DonetBase.h \
  mesh/PartnerList.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  mesh/donet/DonetPeer.h \
  global/churn/IChurnGenerator.h \
  global/observer/MeshOverlayObserver.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h \
  global/ActivePeerInfo.h
$O/mesh/donet/DonetSource.o: mesh/donet/DonetSource.cc \
  mesh/BufferMap.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  common/Dispatcher.h \
  ../../inet/src/base/ModuleAccess.h \
  global/logger/Logger.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  payload/Forwarder.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  global/AppSetting.h \
  mesh/NeighborInfo.h \
  global/ActivePeerTable.h \
  common/CommBase.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  mesh/donet/DonetBase.h \
  mesh/PartnerList.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  ../../inet/src/networklayer/common/InterfaceTableAccess.h \
  global/observer/MeshOverlayObserver.h \
  global/churn/IChurnGenerator.h \
  common/DpControlInfo_m.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/networklayer/ipv4/IPv4InterfaceData.h \
  ../../inet/src/base/INETDefs.h \
  mesh/donet/DonetSource.h \
  global/AppSettingDonet.h \
  global/ActivePeerInfo.h
$O/mesh/donet/RandomScheduling.o: mesh/donet/RandomScheduling.cc \
  mesh/BufferMap.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  payload/PlayerBase.h \
  common/Dispatcher.h \
  ../../inet/src/base/ModuleAccess.h \
  global/logger/Logger.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  payload/Forwarder.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  global/AppSetting.h \
  mesh/NeighborInfo.h \
  global/ActivePeerTable.h \
  common/CommBase.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  mesh/donet/DonetBase.h \
  mesh/PartnerList.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  mesh/donet/DonetPeer.h \
  global/churn/IChurnGenerator.h \
  global/observer/MeshOverlayObserver.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h \
  global/ActivePeerInfo.h
$O/payload/ChunkGenerator.o: payload/ChunkGenerator.cc \
  ../../inet/src/base/INotifiable.h \
  mesh/BufferMap.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  global/AppSetting.h \
  global/ActivePeerTable.h \
  ../../inet/src/base/NotificationBoard.h \
  payload/ChunkGenerator.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h
$O/payload/Forwarder.o: payload/Forwarder.cc \
  ../../inet/src/base/INotifiable.h \
  mesh/BufferMap.h \
  ../../inet/src/base/NotifierConsts.h \
  ../../inet/src/applications/udpapp/UDPAppBase.h \
  payload/VideoChunkPacket_m.h \
  common/Dispatcher.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  payload/Forwarder.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/common/InterfaceToken.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  common/CommBase.h \
  global/ActivePeerTable.h \
  ../../inet/src/networklayer/common/IInterfaceTable.h \
  ../../inet/src/linklayer/contract/MACAddress.h \
  ../../inet/src/networklayer/common/InterfaceEntry.h \
  ../../inet/src/base/NotificationBoard.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  global/churn/IChurnGenerator.h \
  common/DpControlInfo_m.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/ActivePeerInfo.h
$O/payload/Player.o: payload/Player.cc \
  ../../inet/src/base/INotifiable.h \
  mesh/BufferMap.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  global/AppSetting.h \
  global/ActivePeerTable.h \
  ../../inet/src/base/NotificationBoard.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  payload/Player.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h
$O/payload/PlayerSimpleSkip.o: payload/PlayerSimpleSkip.cc \
  mesh/BufferMap.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  payload/PlayerBase.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  payload/PlayerSimpleSkip.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  global/AppSetting.h \
  global/ActivePeerTable.h \
  ../../inet/src/base/NotificationBoard.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h
$O/payload/VideoBuffer.o: payload/VideoBuffer.cc \
  mesh/BufferMap.h \
  ../../inet/src/base/INotifiable.h \
  ../../inet/src/base/NotifierConsts.h \
  payload/VideoChunkPacket_m.h \
  ../../inet/src/base/ModuleAccess.h \
  ../../inet/src/networklayer/contract/IPv6Address.h \
  common/PeerStreamingPacket_m.h \
  ../../inet/src/networklayer/contract/IPAddress.h \
  ../../inet/src/networklayer/contract/IPvXAddress.h \
  global/AppSetting.h \
  global/ActivePeerTable.h \
  ../../inet/src/base/NotificationBoard.h \
  payload/VideoBufferListener.h \
  payload/VideoBuffer.h \
  common/AppCommon.h \
  global/churn/IChurnGenerator.h \
  global/statistic/GlobalStatistic.h \
  mesh/MeshPeerStreamingPacket_m.h \
  ../../inet/src/base/INETDefs.h \
  global/AppSettingDonet.h
$O/payload/VideoBufferListener.o: payload/VideoBufferListener.cc \
  payload/VideoBufferListener.h
$O/payload/VideoChunkPacket_m.o: payload/VideoChunkPacket_m.cc \
  payload/VideoChunkPacket_m.h \
  common/PeerStreamingPacket_m.h
$O/tree/TreePeerStreamingPacket_m.o: tree/TreePeerStreamingPacket_m.cc \
  tree/TreePeerStreamingPacket_m.h \
  common/PeerStreamingPacket_m.h
