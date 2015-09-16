#ifndef __STPAPPS_HH__
#define __STPAPPS_HH__

//#include "Controller.hh"
#include <fluid/of10msg.hh>
#include <fluid/util/ethaddr.hh>
#include <fluid/of13msg.hh>

#include <iostream>
#include <limits.h>
#include <set>
#include <list>
#include <sys/time.h>

using namespace ns3;

#define LLDP_DISCOVERY_ADDRESS "01:80:c2:00:00:0e"
#define NULL_ADDRESS "00:00:00:00:00:00"
#define IPV4_ETHERTYPE_DATAGRAM_FIELD 0x0800
#define LLDP_FLOOD_INTERVAL 100
#define SCHEDULE_TIME_DELAY 2

int is_big_endian(void)
{
  uint32_t i = 0x01020304;
  char c[4];
  memcpy(c, &i, 4);
  return c[0] == 1;
}

void* byteMemcpy(void *dst, void *src,size_t size)
{
  if(is_big_endian())
    return memcpy(dst,src,size);

  for(uint32_t i = 0; i < size; ++i){
    memcpy((uint8_t*)dst + i,(uint8_t*)src + (size - i - 1),1);
  }
  return dst;
} 

static uint32_t crc32table[256] = {
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
    0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
    0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
    0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
    0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
    0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
    0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
    0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,0xB6662D3D,
    0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
    0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
    0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
    0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
    0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
    0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
    0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
    0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
    0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
    0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
    0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
    0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
    0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
    0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
    0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
    0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
    0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
    0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
    0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
    0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
    0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
    0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
    0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D
};

void updateTopologyGlobal (void* arg); 
void flood_discovery_packetsGlobal (void* arg);

class STPApps: public MultiLearningSwitch {

public:
    STPApps (void) {
      xIDs = 0;
      lldpAddress = fluid_msg::EthAddress(LLDP_DISCOVERY_ADDRESS);
      state = STPS_FREE;
    }

    //Structure definitions

    //Controller state
    enum STPState {
      STPS_FREE,
      STPS_SCHEDULEDTOPOLOGY,
    };
    STPState state;

    //A link that the stp switch uses to defined connection in STP
    //Contains a source and destionation datapath id, a destination port, and a delayvalue
    struct link{
      link () : srcDPID(0), dstDPID(0), dstPort(0), delay(0) {}
      uint64_t srcDPID;
      uint64_t dstDPID;
      uint16_t dstPort;
      uint64_t delay;
    };

    struct cmp_link {
      bool operator() (const link &lhs, const link &rhs) const
      {
        if(lhs.srcDPID == rhs.srcDPID)
          return lhs.dstDPID < rhs.dstDPID;
        return lhs.srcDPID < rhs.srcDPID;
      }
    };

    //Table definition here
    //List of switches. Sorted by DPID. Needs to get all the links on that 
    struct STPSwitch {
      bool alive = true;
      Ptr<SdnConnection> switchConn;
      uint64_t DPID;
      uint64_t discoverySentTime;
      std::set<link, cmp_link> links;
      std::vector<fluid_msg::of10::Port> hostCandidates;
    };

    // Structures for static function void* args
    struct UpdateTopoStruct {
      STPApps* app;
    };

    struct FloodDiscoStruct {
      STPApps* app;
      Ptr<SdnConnection> ofconn;
      STPSwitch *switchInfo;
      std::vector<fluid_msg::of10::Port> ports;
    };

    int xIDs;

    int GenerateXId (void) {
      xIDs++; 
      return xIDs; 
    }

    //LLDP address in ethaddress form
    fluid_msg::EthAddress lldpAddress;
    bool started = false;

    struct cmp_DPID {
      //comparator by a switches DPID
      bool operator() (const STPSwitch* lhs, const STPSwitch* rhs) const
      {
        return lhs->DPID < rhs->DPID;
      }
    };
    std::set <STPSwitch*, cmp_DPID> switches;
    std::set<link, cmp_link> currentTopology;
    uint64_t initializeStartTime;
    uint64_t initializeEndTime;

    virtual void event_callback(ControllerEvent* ev) {
        if(ev->get_type() == EVENT_SWITCH_UP) {
          if(!started){
            started = true;
            initializeStartTime = getCurrentTime ();
          }
          SwitchUpEvent* su = static_cast<SwitchUpEvent*>(ev);
          fluid_msg::of10::FeaturesReply *fReply = new fluid_msg::of10::FeaturesReply();
          fReply->unpack(su->data);
          std::cout << "STPApp: Booting up. Found " << fReply->ports().size() << " ports." << std::endl;
          //Initialize new switch
          STPSwitch* newSwitch = new STPSwitch();
          newSwitch->switchConn = ev->ofconn;
          newSwitch->DPID = fReply->datapath_id();
          newSwitch->hostCandidates = fReply->ports();
          //Add Switch to table
          switches.insert(newSwitch);
          for(uint32_t i = 0; i < fReply->ports().size(); ++i){
            fluid_msg::of10::Port currentPort = fReply->ports()[i];
            turn_off_flooding_via_port_mod(fReply, ev->ofconn, currentPort.port_no());
            // Probably have to do with changing the ports while using the iterator, however
            // the messages didn't get sent out until after the standard SOL delay.... Hmmmmmmm
          }
          receive_discovery_packets(ev->ofconn);
          this->flood_discovery_packets(ev->ofconn, newSwitch, fReply->ports());

          MultiLearningSwitch::event_callback(ev);
        }

        else if (ev->get_type() == EVENT_PACKET_IN) {
          STPSwitch* thisSwitch = findSTPSwitch(switches,ev->ofconn);
          PacketInEvent* pi = static_cast<PacketInEvent*>(ev);
          fluid_msg::of10::PacketIn *ofpi = new fluid_msg::of10::PacketIn();
          uint64_t dst = 0, src = 0;
          ofpi->unpack(pi->data);
          memcpy(((uint8_t*) &dst + 2), (uint8_t*) ofpi->data(), 6);
          fluid_msg::EthAddress dstAddress ((uint8_t*)&dst + 2);
          memcpy(((uint8_t*) &src + 2), (uint8_t*) ofpi->data() + 6, 6);
          fluid_msg::EthAddress srcAddress ((uint8_t*)&src + 2);
          uint16_t ethType = 0;
          memcpy(((uint8_t*)&ethType), (uint8_t*) ofpi->data() + 12, 2);
          if(dstAddress == lldpAddress){ //If we received a discovery packet
            uint64_t dpid = 0; //Get the DPID src from the packet
            memcpy(&dpid, (uint8_t*) ofpi->data() + 14,sizeof(uint64_t));
            STPSwitch* srcSwitch = findSTPSwitchOnDPID(switches,dpid);
            if(srcSwitch && thisSwitch){
              //Make a new link
              link newLink;
              newLink.srcDPID = srcSwitch->DPID;
              newLink.dstDPID = thisSwitch->DPID;
              newLink.dstPort = ofpi->in_port();
              newLink.delay = getCurrentTime () - srcSwitch->discoverySentTime;

              if(!srcSwitch->links.insert(newLink).second){ //If there's allready a link listed in here
                //We need to re-add the port if this happens
                link oldLink = *(srcSwitch->links.find(newLink));
                if(oldLink.delay > newLink.delay){
                  srcSwitch->links.erase(oldLink);
                  srcSwitch->links.insert(newLink);
                  scheduleNewTopology(ev);
                }
              }
              else{
                scheduleNewTopology(ev);
              }
              //Remove the hostCandidate from the "externals list". It's now an internal link.
              std::vector<fluid_msg::of10::Port>::iterator port = findPortViaPortNo(thisSwitch, ofpi->in_port());
              if(port != thisSwitch->hostCandidates.end()){
                thisSwitch->hostCandidates.erase(port);
              }
              // Remove port from vector of host candidates for this particular connection              
            }
            return;
          }
          else if(thisSwitch && findlinkInTopology(thisSwitch,ofpi->in_port()) == currentTopology.end()) { //If not in topology
            std::vector<fluid_msg::of10::Port>::iterator port = findPortViaPortNo(thisSwitch, ofpi->in_port());
            if(port != thisSwitch->hostCandidates.end()){
              MultiLearningSwitch::event_callback(ev);
              return;
              }
            //Is the source a switch we know about?
            L2TABLE* knownhosts = get_l2table(ev->ofconn);
            L2TABLE::iterator hostsIterator = get_l2table(ev->ofconn)->find(dst);
            if(hostsIterator != knownhosts->end()){
              MultiLearningSwitch::event_callback(ev);
            }
            return;
          }
          //Else if it is in the topology, then we're ok
          MultiLearningSwitch::event_callback(ev);
        }

        else if(ev->get_type() == EVENT_FLOW_REMOVED){
          FlowRemovedEvent* fr = static_cast<FlowRemovedEvent*>(ev);
          fluid_msg::of10::FlowRemoved *offr = new fluid_msg::of10::FlowRemoved();
          offr->unpack(fr->data);
        }

        else if(ev->get_type() == EVENT_PORT_STATUS){
          PortStatusEvent* ps = static_cast<PortStatusEvent*>(ev);
          fluid_msg::of10::PortStatus *ofps = new fluid_msg::of10::PortStatus();
          ofps->unpack(ps->data);
          //Get port status down
          if(ofps->reason() == fluid_msg::of10::OFPPR_ADD){
            //Add links
            scheduleNewTopology(ev);
          }
          else if(ofps->reason() == fluid_msg::of10::OFPPR_DELETE){
            //Delete links
            scheduleNewTopology(ev);
          }
        }

    }

    //Turn off the flood flag
    void turn_off_flooding_via_port_mod(fluid_msg::of10::FeaturesReply *fr,
    		Ptr<SdnConnection> ofconn,
			uint16_t port_no){
        uint32_t configureValues = fluid_msg::of10::OFPPC_NO_FLOOD;
        uint32_t configureMask = fluid_msg::of10::OFPPC_NO_FLOOD;
        fluid_msg::of10::PortMod pm;
        pm.xid(fr->xid());
        pm.port_no(port_no);
        pm.config(configureValues);
        pm.mask(configureMask);
        pm.advertise(0);
        uint8_t* buffer = pm.pack();
        ofconn->send(buffer, pm.length());
        fluid_msg::OFMsg::free_buffer(buffer);
    }
    //Discover all of our special packets please
    void receive_discovery_packets(Ptr<SdnConnection> ofconn){
      //Make a new rule
      fluid_msg::of10::FlowMod fm(GenerateXId(), //XID
          123,             //cookie
          fluid_msg::of10::OFPFC_ADD, // Command
          100,              // Idle Timeout
          500,             // Hard Timeout
          -1,              // No corresponding buffer on switch
          100,             // Priority
          0,               // outport (not really using them)
          fluid_msg::of10::OFPFF_SEND_FLOW_REM & fluid_msg::of10::OFPFF_CHECK_OVERLAP); // TODO Note we don't use these flags.
      //Match on LLDP Discovery destination in ethernet frame, and send the packet back to us the controller
      fluid_msg::of10::Match m;
      m.dl_dst(lldpAddress);
      fluid_msg::of10::OutputAction act(fluid_msg::of10::OFPP_CONTROLLER, 1024); //Need to update controller to handle more special out ports
      //Send out the flowmod
      fm.match(m);
      fm.add_action(act);
      uint8_t* buffer = fm.pack();
      ofconn->send(buffer, fm.length());
      fluid_msg::OFMsg::free_buffer(buffer);
    }

    //Send out our special LLDP discovery packets to find delays
    void flood_discovery_packets(Ptr<SdnConnection> ofconn, STPSwitch* switchInfo, std::vector<fluid_msg::of10::Port> ports){
      switchInfo->discoverySentTime = getCurrentTime ();

      uint8_t DPIDbuff[sizeof(uint64_t)];
      memcpy((uint8_t*) &DPIDbuff, (uint8_t*)&switchInfo->DPID, sizeof(uint64_t));
      fluid_msg::EthAddress dstAddress(LLDP_DISCOVERY_ADDRESS);
      fluid_msg::EthAddress srcAddress(NULL_ADDRESS);
      uint32_t packetBuffLen = sizeof(DPIDbuff) + 18;
      uint8_t *packetBuff = (uint8_t*)malloc(packetBuffLen);
      
      uint16_t ethType = IPV4_ETHERTYPE_DATAGRAM_FIELD;
      memcpy( packetBuff + 0, ((uint8_t*) dstAddress.get_data()), 6);
      memcpy( packetBuff + 6, ((uint8_t*) srcAddress.get_data()), 6);
      byteMemcpy( packetBuff + 12, &ethType, 2); //We have to do a byte memcpy here because of ethernet deserializing doing a hton
      memcpy( packetBuff + 14, &DPIDbuff, sizeof(DPIDbuff));
      uint32_t trailer = CRC32Calculate(packetBuff, packetBuffLen);
      memcpy( packetBuff + 14 + sizeof(DPIDbuff), &trailer, sizeof(uint32_t));
      
      fluid_msg::of10::PacketOut po(GenerateXId(), -1, 0);
      po.data(packetBuff, packetBuffLen);
      
      for(std::vector<fluid_msg::of10::Port>::iterator port_it = ports.begin(); port_it != ports.end(); ++port_it){
        fluid_msg::of10::OutputAction act(port_it->port_no(), 1024); //Need to update controller to handle more special out ports
        po.add_action(act);
        uint8_t* buffer = po.pack();
        ofconn->send(buffer, po.length()); //Does this just send the buffer out and we can reuse it?
        fluid_msg::OFMsg::free_buffer(buffer);
      }
      if(switchInfo->alive){
        FloodDiscoStruct* floodDisco = new FloodDiscoStruct();
        floodDisco->app = this;
        floodDisco->ofconn = ofconn;
        floodDisco->switchInfo = switchInfo;
        floodDisco->ports = ports;
        ofconn->add_timed_callback (flood_discovery_packetsGlobal, LLDP_FLOOD_INTERVAL, (void *)floodDisco);
      }
      free (packetBuff);
    }
    
    void block_port_via_flowmod(Ptr<SdnConnection> conn,uint16_t in_port){
      //Make a new rule
      fluid_msg::of10::FlowMod fm(GenerateXId(), //XID
      123,             //cookie
      fluid_msg::of10::OFPFC_ADD, // Command
      100,             // Idle Timeout
      500,             // Hard Timeout
      -1,              // No corresponding buffer on switch
      200,             // Higher prioritiy than our parent class
      0,               // outport for delete commands
      fluid_msg::of10::OFPFF_SEND_FLOW_REM & fluid_msg::of10::OFPFF_CHECK_OVERLAP); // TODO Note we don't use these flags in the simulation
      fluid_msg::of10::Match m;
      m.in_port(in_port);
      //No action provided, meaning we drop the packet
      //Send out the flowmod
      uint8_t* buffer;
      fm.match(m);
      buffer = fm.pack();
      conn->send(buffer, fm.length());
      fluid_msg::OFMsg::free_buffer(buffer);
    }
    
    //Finds the new STP and creates the difference in topologies. Sends the new flows out into the world
    void updateTopology(void){
      //Find our updated topology
      std::set<link, cmp_link> newTopology = generateTopology();
      //Get the added and removed links
      std::pair<std::set<link, cmp_link>,std::set<link, cmp_link> > addedRemoved = generateTopologyDiff(currentTopology,newTopology);
      //Send added new flows
      for(std::set<link>::iterator i = addedRemoved.first.begin(); i != addedRemoved.first.end(); i++){
        updateLink(*i, true);
      }
      //Send deleted old flows
      for(std::set<link>::iterator i = addedRemoved.second.begin(); i != addedRemoved.second.end(); i++){
        updateLink(*i, false);
      }
      //Open all the outside host ports
      openTheHostGateways();
      //Confirm the new topology
      currentTopology = newTopology;
      state = STPS_FREE;

      initializeEndTime = getCurrentTime ();

      return;
    }
    
    std::set<link, cmp_link> generateTopology() //Return some structure at some point
    {
      std::set<STPSwitch*, cmp_DPID> processed;
      std::list<STPSwitch*> pending;
      std::set<link, cmp_link> newTopology;
      pending.insert(pending.begin(), switches.begin(), switches.end()); //Start with root node
      while(!pending.empty()){ //Continue untill all pending is empty
        STPSwitch* currentSwitch = pending.front();
        pending.pop_front();
        std::set<STPSwitch*, cmp_DPID> neighbors;
        //Check every link on this switch
        for(std::set<link>::iterator i = currentSwitch->links.begin(); i != currentSwitch->links.end(); ++i){
          bool dstAlreadyInGraph = false;
          for(std::set<link, cmp_link>::iterator j = newTopology.begin(); j != newTopology.end(); ++j){
            if(i->dstDPID == (*j).dstDPID){
              dstAlreadyInGraph = true;
              break;
            }
          }
          //If the link is connecting us somewhere new
          if(!dstAlreadyInGraph){
            //Insert that link into the topology
            link otherLink = getPairedLink(*i);
            newTopology.insert(*i);
            if (otherLink.dstPort != 0) {
              newTopology.insert(otherLink);
            }
            for(std::list<STPSwitch*>::iterator j = pending.begin(); j != pending.end(); ++j){
            //Mark that destination as a neighbor, and remove from pending
              if(i->dstDPID == (*j)->DPID){
                neighbors.insert(*j);
                j = pending.erase(j);
              }
            }
          }
        }
        processed.insert(currentSwitch);
        pending.insert(pending.begin(),neighbors.begin(),neighbors.end()); //Inserts our now sorted neighbors into the front of pending
      }
      return newTopology;
    }
    
    //First element is the added links. 2nd element is the removed links
    std::pair<std::set<link, cmp_link>,std::set<link, cmp_link> > generateTopologyDiff(std::set<link, cmp_link> oldTopo, std::set<link, cmp_link> newTopo){
      std::set<link, cmp_link> added;
      std::set<link, cmp_link> removed;
      //Find all elements in new but not old
      for(std::set<link, cmp_link>::iterator i = newTopo.begin(); i != newTopo.end(); ++i){
        std::set<link, cmp_link>::iterator match = oldTopo.find(*i);
        if(match == oldTopo.end()){
          added.insert(*i);
        }
      }
      //Find all elemetns in old but not new
      for(std::set<link, cmp_link>::iterator i = oldTopo.begin(); i != oldTopo.end(); ++i){
        std::set<link, cmp_link>::iterator match = newTopo.find(*i);
        if(match == newTopo.end()){
          removed.insert(*i);
        }
      }
      std::pair<std::set<link, cmp_link>,std::set<link ,cmp_link> > returnValue(added,removed);
      return returnValue;
    }
    
    //Allow or disallow different ports based on our spanning tree
    void updateLink(link link, bool added){
      STPSwitch *dstSwitch = findSTPSwitchOnDPID(switches,link.dstDPID);
      updateNoFloodOnPort(dstSwitch, link.dstPort, !added);
    }
    
    // Returns the other single-directional link, creating a bi-directional pair
    link getPairedLink(link linkA){
      for (std::set<STPSwitch*, cmp_DPID>::iterator i = switches.begin(); i != switches.end(); ++i) {
        for(std::set<link, cmp_link>::iterator j = (*i)->links.begin(); j != (*i)->links.end(); ++j){
          if (j->srcDPID == linkA.dstDPID && j->dstDPID == linkA.srcDPID)
          {
            link linkB = *j;
            return linkB;
          }
        }
      }
      return link();
    }
    
    void openTheHostGateways(){
      for(std::set<STPSwitch*,cmp_DPID>::iterator i = switches.begin(); i != switches.end(); ++i){
        for(std::vector<fluid_msg::of10::Port>::iterator j = (*i)->hostCandidates.begin(); j != (*i)->hostCandidates.end(); ++j){
          updateNoFloodOnPort(*i, j->port_no(), false);
        }
      }
    }
    void updateNoFloodOnPort(STPSwitch* thisSwitch, uint16_t port_no, bool floodFlag){
      //Send a portmod
      fluid_msg::of10::PortMod pm(
      GenerateXId(), //XID
      port_no, //Port number
      fluid_msg::EthAddress(), //No hardware address
      floodFlag ? fluid_msg::of10::OFPPC_NO_FLOOD : 0,//Config
      fluid_msg::of10::OFPPC_NO_FLOOD,//Mask
      fluid_msg::of10::OFPPC_NO_FLOOD);//Advertised
      //Send out the portmod
      uint8_t* buffer = pm.pack();
      thisSwitch->switchConn->send(buffer, pm.length());
      fluid_msg::OFMsg::free_buffer(buffer);
    }
    
    //Schedules a new topology update into the future if there's not one scheduled allready
    void scheduleNewTopology(ControllerEvent* ev){
      if(state == STPS_FREE){
        state = STPS_SCHEDULEDTOPOLOGY;

        UpdateTopoStruct* uts = new UpdateTopoStruct ();
        uts->app = this;
        ev->ofconn->add_timed_callback (updateTopologyGlobal, SCHEDULE_TIME_DELAY, (void *)uts);
        //Simulator::Schedule(Seconds(SCHEDULE_TIME_DELAY),&STPApps::updateTopology,this);
      }
    }
    //Find an STPSwitch based on an sdn connection
    STPSwitch* findSTPSwitch(std::set<STPSwitch*, cmp_DPID> list, Ptr<SdnConnection> sdnConn){
      for(std::set<STPSwitch*, cmp_DPID>::iterator i = list.begin(); i != list.end(); i++){
        if((*i)->switchConn->get_id() == sdnConn->get_id())
          return *i;
      }
      return 0;
    }
    //Find STPSwitch based on a dpid
    STPSwitch* findSTPSwitchOnDPID(std::set<STPSwitch*, cmp_DPID> list, uint64_t dpid){
      for(std::set<STPSwitch*, cmp_DPID>::iterator i = list.begin(); i != list.end(); i++){
        if((*i)->DPID == dpid)
          return *i;
      }
      return 0;
    }
    //Find a link in the link vector based on a destination switch and it's port number (which should be unique)
    std::set<link, cmp_link>::iterator findlinkInTopology(STPSwitch* searchSwitch,uint16_t portno){
      for(std::set<link, cmp_link>::iterator i = currentTopology.begin(); i != currentTopology.end(); i++){
        if((*i).dstDPID == searchSwitch->DPID && (*i).dstPort == portno){
          return i;
        }
      }
      return currentTopology.end();
    }
    //Find a port from the host candidates list on an STPSwitch
    std::vector<fluid_msg::of10::Port>::iterator findPortViaPortNo(STPSwitch* thisSwitch, uint16_t port_no){
      std::vector<fluid_msg::of10::Port> *hostCandidates = &(thisSwitch->hostCandidates);
      std::vector<fluid_msg::of10::Port>::iterator i = hostCandidates->begin();
      while(i != hostCandidates->end()){
        if (i->port_no() == port_no){
          return i;
        }
        ++i;
      }
      return i;
    }

    uint32_t CRC32Calculate (const uint8_t *data, int length) {
     uint32_t crc = 0xffffffff;

     while (length--) {
       crc = (crc >> 8) ^ crc32table[(crc & 0xFF) ^ *data++];
     }
     return ~crc;
    }
    
    // Get the current time in ms
    uint64_t getCurrentTime () {
      return Simulator::Now().GetMicroSeconds();
//      struct timeval curTime;
//      gettimeofday(&curTime, NULL);
//      return curTime.tv_sec * 1000000 + curTime.tv_usec;
    }

    uint64_t getInitializationTime(){
      if(initializeStartTime != 0 && initializeEndTime != 0){
        return initializeEndTime - initializeStartTime;
      }
      return 0;
    }
};

void updateTopologyGlobal (void* arg) {
  STPApps::UpdateTopoStruct* uts = static_cast<STPApps::UpdateTopoStruct*>(arg);

  uts->app->updateTopology ();
}

void flood_discovery_packetsGlobal (void* arg) {
  STPApps::FloodDiscoStruct* fds = static_cast<STPApps::FloodDiscoStruct*>(arg);

  fds->app->flood_discovery_packets (fds->ofconn, fds->switchInfo, fds->ports);
}

#endif
