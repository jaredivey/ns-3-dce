#ifndef __MSGAPPS_HH__
#define __MSGAPPS_HH__

//#include "Controller.hh"
#include <fluid/of10msg.hh>
#include <fluid/util/ethaddr.hh>
#include <fluid/of13msg.hh>

using namespace ns3;

class MultiLearningSwitch: public BaseLearningSwitch {
public:
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
            uint16_t in_port = fluid_msg::of10::OFPP_NONE;
            if (ofversion == fluid_msg::of10::OFP_VERSION) {
                fluid_msg::of10::PacketIn *ofpi = new fluid_msg::of10::PacketIn();
                ofpip = ofpi;
                ofpi->unpack(pi->data);
                memcpy(((uint8_t*) &dst + 2), (uint8_t*) ofpi->data(), 6);
                memcpy(((uint8_t*) &src + 2), (uint8_t*) ofpi->data() + 6, 6);
                memcpy(((uint8_t*)&ethType), (uint8_t*) ofpi->data() + 12, 2);
                in_port = ofpi->in_port();
            }
            else if (ofversion == fluid_msg::of13::OFP_VERSION) {
                fluid_msg::of13::PacketIn *ofpi = new fluid_msg::of13::PacketIn();
                ;
                ofpip = ofpi;
                ofpi->unpack(pi->data);
                memcpy(((uint8_t*) &dst + 2), (uint8_t*) ofpi->data(), 6);
                memcpy(((uint8_t*) &src + 2), (uint8_t*) ofpi->data() + 6, 6);
                if (ofpi->match().in_port() == NULL) {
                    return;
                }
                in_port = ofpi->match().in_port()->value();
            }

            // Learn the source
            (*l2table)[src] = in_port;

            // Try to find the destination
            L2TABLE::iterator it = l2table->find(dst);
            if (it == l2table->end()) {
                if (ofversion == fluid_msg::of10::OFP_VERSION) {
                    flood10(*((fluid_msg::of10::PacketIn*) ofpip), ev->ofconn);
                    delete (fluid_msg::of10::PacketIn*) ofpip;
                }
                else if (ofversion == fluid_msg::of13::OFP_VERSION) {
                    flood13(*((fluid_msg::of13::PacketIn*) ofpip), ev->ofconn, in_port);
                    delete (fluid_msg::of13::PacketIn*) ofpip;
                }
                return;
            }

            if (ofversion == fluid_msg::of10::OFP_VERSION) {
                install_flow_mod10(*((fluid_msg::of10::PacketIn*) ofpip), ev->ofconn, src,
                    dst, it->second);
                delete (fluid_msg::of10::PacketIn*) ofpip;
            }
            else if (ofversion == fluid_msg::of13::OFP_VERSION) {
                install_flow_mod13(*((fluid_msg::of13::PacketIn*) ofpip), ev->ofconn, src,
                    dst, it->second);
                delete (fluid_msg::of13::PacketIn*) ofpip;
            }
        }
        else if (ev->get_type() == EVENT_SWITCH_UP) {
            BaseLearningSwitch::event_callback(ev);
            if (ofversion == fluid_msg::of13::OFP_VERSION) {
                install_default_flow13(ev->ofconn);
            }
        }

        else {
            BaseLearningSwitch::event_callback(ev);
        }
    }

    void install_flow_mod10(fluid_msg::of10::PacketIn &pi, Ptr<SdnConnection> ofconn, uint64_t src, uint64_t dst, uint16_t out_port) {
        //std::cout << "flowmod10" << std::endl;
        // Flow mod message
        uint8_t* buffer;
        /* Messages constructors allow to add all 
         values in a row. The fields order follows
         the specification */
        fluid_msg::of10::FlowMod fm(pi.xid(),  //xid
            123, // cookie
            fluid_msg::of10::OFPFC_ADD, // command
            100, // idle timeout
            500, // hard timeout
            100, // priority
            pi.buffer_id(), //buffer id
            0, // outport
            0); // flags
        fluid_msg::of10::Match m;
        m.dl_src(((uint8_t*) &src) + 2);
        m.dl_dst(((uint8_t*) &dst) + 2);
        fm.match(m);
        fluid_msg::of10::OutputAction act(out_port, 1024);
        fm.add_action(act);
        buffer = fm.pack();
        ofconn->send(buffer, fm.length());
        if((int32_t)pi.buffer_id() == -1){ //If buffer id == -1, send back the packet as an action to send as well
          fluid_msg::of10::PacketOut po(pi.xid(), pi.buffer_id(), pi.in_port());
          po.data(pi.data(), pi.data_len());
          po.add_action(act);
          ofconn->send(po.pack(), po.length());
        }
        fluid_msg::OFMsg::free_buffer(buffer);
    }

    void flood10(fluid_msg::of10::PacketIn &pi, Ptr<SdnConnection> ofconn) {
        fluid_msg::of10::PacketOut po(pi.xid(), pi.buffer_id(), pi.in_port());
        /*Add Packet in data if the packet was not buffered*/
        if ((int32_t)pi.buffer_id() == -1) {
            po.data(pi.data(), pi.data_len());
        }
        fluid_msg::of10::OutputAction act(fluid_msg::of10::OFPP_FLOOD, 1024);
        po.add_action(act);

        uint8_t* buffer = po.pack();
        ofconn->send(buffer, po.length());
        fluid_msg::OFMsg::free_buffer(buffer);
    }

    void install_default_flow13(Ptr<SdnConnection> ofconn) {
        fluid_msg::of13::FlowMod fm(42, 0, 0xffffffffffffffff, 0, fluid_msg::of13::OFPFC_ADD, 0, 0, 0,
            0xffffffff, 0, 0, 0);
        fluid_msg::of13::OutputAction *act = new fluid_msg::of13::OutputAction(fluid_msg::of13::OFPP_CONTROLLER,
            fluid_msg::of13::OFPCML_NO_BUFFER);
        fluid_msg::of13::ApplyActions *inst = new fluid_msg::of13::ApplyActions();
        inst->add_action(act);
        fm.add_instruction(inst);

        uint8_t* buffer = fm.pack();
        ofconn->send(buffer, fm.length());
        fluid_msg::OFMsg::free_buffer(buffer);
    }

    void install_flow_mod13(fluid_msg::of13::PacketIn &pi, Ptr<SdnConnection> ofconn,
        uint64_t src, uint64_t dst, uint32_t out_port) {
        // Flow mod message
        /*You can also set the message field using
         class methods which have the same names from
         the field present on the specification*/
        fluid_msg::of13::FlowMod fm;
        fm.xid(pi.xid());
        fm.cookie(123);
        fm.cookie_mask(0xffffffffffffffff);
        fm.table_id(0);
        fm.command(fluid_msg::of13::OFPFC_ADD);
        fm.idle_timeout(100);
        fm.hard_timeout(500);
        fm.priority(100);
        fm.buffer_id(pi.buffer_id());
        fm.out_port(0);
        fm.out_group(0);
        fm.flags(0);
        fluid_msg::of13::EthSrc fsrc(((uint8_t*) &src) + 2);
        fluid_msg::of13::EthDst fdst(((uint8_t*) &dst) + 2);
        fm.add_oxm_field(fsrc);
        fm.add_oxm_field(fdst);
        fluid_msg::of13::OutputAction act(out_port, 1024);
        fluid_msg::of13::ApplyActions inst;
        inst.add_action(act);
        fm.add_instruction(inst);

        uint8_t* buffer = fm.pack();
        ofconn->send(buffer, fm.length());
        fluid_msg::OFMsg::free_buffer(buffer);
        fluid_msg::of13::Match m;
        fluid_msg::of13::MultipartRequestFlow rf(2, 0x0, 0, fluid_msg::of13::OFPP_ANY, fluid_msg::of13::OFPG_ANY,
            0x0, 0x0, m);
        buffer = rf.pack();
        ofconn->send(buffer, rf.length());
        fluid_msg::OFMsg::free_buffer(buffer);
    }

    void flood13(fluid_msg::of13::PacketIn &pi, Ptr<SdnConnection> ofconn, uint32_t in_port) {
        fluid_msg::of13::PacketOut po(pi.xid(), pi.buffer_id(), in_port);
        /*Add Packet in data if the packet was not buffered*/
        if ((int32_t)pi.buffer_id() == -1) {
            po.data(pi.data(), pi.data_len());
        }
        fluid_msg::of13::OutputAction act(fluid_msg::of13::OFPP_FLOOD, 1024); // = new of13::OutputAction();
        po.add_action(act);

        uint8_t* buffer = po.pack();
        ofconn->send(buffer, po.length());
        fluid_msg::OFMsg::free_buffer(buffer);
    }
};

#endif
