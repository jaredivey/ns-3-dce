#ifndef __FIREWALLAPPS_HH__
#define __FIREWALLAPPS_HH__

//#include "Controller.hh"
#include <fluid/of10msg.hh>
#include <fluid/util/ethaddr.hh>
#include <fluid/of13msg.hh>

#include "MsgApps.hh"

using namespace ns3;

class Firewall : public MultiLearningSwitch {
public:
    typedef std::pair<uint64_t,uint64_t> Pathway;
    struct cmp_pathway
    {
      bool operator() (const Pathway &lhs, const Pathway &rhs) const
      {
        if(lhs.first < rhs.first)
          return false;
        else if(lhs.first > rhs.first)
          return true;
        else{
          if(lhs.second < rhs.second)
            return false;
          else if(lhs.second > rhs.second)
            return true;
          else
            return false;
        }
      }
    };
    typedef std::set<Pathway, cmp_pathway> PathwaySet;
    
    PathwaySet blacklist; //The list of source to destination by dls that we allow
    
    Firewall(std::string fileName){ //Must read in a file to construct a firewall correctly
        std::ifstream mappingFile;
        mappingFile.open(fileName.c_str());
        if (mappingFile.is_open()){ //check if we opened the file
            std::string srcAddressString;
            std::string dstAddressString;
            while(!mappingFile.eof()){
                getline(mappingFile, srcAddressString, ','); //Expecting CSV values
                getline(mappingFile, dstAddressString, '\n');
                fluid_msg::EthAddress srcAddress(srcAddressString);
                fluid_msg::EthAddress dstAddress(dstAddressString);
                uint64_t src = 0, dst = 0;
                memcpy(((uint8_t*)&dst + 2), dstAddress.get_data(), 6);
                memcpy(((uint8_t*)&src + 2), srcAddress.get_data(), 6);
                blacklist.insert(std::make_pair(src, dst)); //Insert the new pair we created
              }
          }
        else{
            NS_ABORT_MSG ("Couldn't read the firewall list at " << fileName);
          }
      }
    
    virtual void event_callback(ControllerEvent* ev) {
        uint8_t ofversion = ev->ofconn->get_version();

        if (ev->get_type() == EVENT_PACKET_IN) {
            L2TABLE* l2table = get_l2table(ev->ofconn);
            if (l2table == NULL) {
                return;
            }
            uint64_t dst = 0, src = 0;
            uint16_t ethType = 0;
            PacketInEvent* pi = static_cast<PacketInEvent*>(ev);
            void* ofpip = NULL;
            if (ofversion == fluid_msg::of10::OFP_VERSION) {
                fluid_msg::of10::PacketIn *ofpi = new fluid_msg::of10::PacketIn();
                ofpip = ofpi;
                ofpi->unpack(pi->data);
                //data = (uint8_t*) ofpi->data();
                memcpy(((uint8_t*)&dst + 2), (uint8_t*) ofpi->data(), 6);
                memcpy(((uint8_t*)&src + 2), (uint8_t*) ofpi->data() + 6, 6);
                memcpy(((uint8_t*)&ethType), (uint8_t*) ofpi->data() + 12, 2);
            }
            else if (ofversion == fluid_msg::of13::OFP_VERSION) {
                fluid_msg::of13::PacketIn *ofpi = new fluid_msg::of13::PacketIn();
                ofpip = ofpi;
                ofpi->unpack(pi->data);
                //data = (uint8_t*) ofpi->data();
                memcpy(((uint8_t*) &dst + 2), (uint8_t*) ofpi->data(), 6);
                memcpy(((uint8_t*) &src + 2), (uint8_t*) ofpi->data() + 6, 6);
                if (ofpi->match().in_port() == NULL) {
                    return;
                }
            }
            //Check the firewall
            PathwaySet::iterator packetInPathway = blacklist.find(std::make_pair(src, dst));
            if(packetInPathway != blacklist.end())
            {
              install_flow_mod_no_action10(*((fluid_msg::of10::PacketIn*) ofpip), ev->ofconn, src, dst);
              //Make a pathway with no action so we'll drop the packet
              return;
            }
        }
        MultiLearningSwitch::event_callback(ev);
    }
    
    void install_flow_mod_no_action10(fluid_msg::of10::PacketIn &pi, Ptr<SdnConnection> ofconn, uint64_t src, uint64_t dst) {
        //std::cout << "flowmod10" << std::endl;
        uint8_t* buffer;
        /* Messages constructors allow to add all 
         values in a row. The fields order follows
         the specification */
        fluid_msg::of10::FlowMod fm(pi.xid(),  //xid
            123, // cookie
            fluid_msg::of10::OFPFC_ADD, // command
            50, // idle timeout
            100, // hard timeout
            100, // priority
            pi.buffer_id(), //buffer id
            0, // outport
            0); // flags
        fluid_msg::of10::Match m;
        m.dl_src(((uint8_t*) &src) + 2);
        m.dl_dst(((uint8_t*) &dst) + 2);
        fm.match(m);
        buffer = fm.pack();
        ofconn->send(buffer, fm.length());
        fluid_msg::OFMsg::free_buffer(buffer);
    }
};

#endif
