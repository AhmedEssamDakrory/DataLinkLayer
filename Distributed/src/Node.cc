//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "Node.h"
#include <fstream>
Define_Module(Node);

void Node::initialize()
{
}



void Node::handleMessage(cMessage *msg)
{
    MyMessage_Base *mmsg = check_and_cast<MyMessage_Base *>(msg);
    if(mmsg->getM_Type() == INIT){
        std::cout << getIndex() <<" --> " << mmsg->getAck()  <<endl;
        //resetting start time
        lastResetTime = msg->getTimestamp();
        // initializing the destination node.
        dest = mmsg->getAck();
        if(dest > getIndex()) --dest;
        initGoBackN();
        enableNetworkLayer();

    } else {
        const simtime_t  messageCreatedAt = mmsg->getTimestamp();
        std::cout<<"Node "<<getIndex()<<endl;
        std::cout << messageCreatedAt << "--" << lastResetTime << endl;
        if(messageCreatedAt >= lastResetTime){
            startGoBackN(mmsg);
        } else{
            EV<<"Ignore messages from last time step" << endl;
        }
    }

}

void Node::initGoBackN()
{
    next_frame_to_send = 0;
    ack_expected = 0;
    frame_expected = 0;
    nbuffered = 0;
    int m = par("m");
    MAX_SEQ = (1 << m) -1; // window size should be less than (2^m)
    buffer = new const char*[MAX_SEQ+1];
    timers = new MyMessage_Base*[MAX_SEQ+1];
    isTimerSet = new bool[MAX_SEQ+1];
    for(int i = 0 ; i < MAX_SEQ+1; ++i){
        isTimerSet[i] = 0;
    }
}

const char* Node::fromNetworkLayer()
{
    //TODO: return random message
    const char* message = "hello $//there//";
    return  message;
}


void Node::startTimer(int seq_num)
{
    MyMessage_Base * mmsg = new MyMessage_Base("");
    mmsg->setM_Type(Timeout);
    mmsg->setSeq_Num(seq_num);
    scheduleAt(simTime() + par("timeout_period"), mmsg);
    if(isTimerSet[seq_num]){
        stopTimer(seq_num);
    }
    isTimerSet[seq_num] = 1;
    timers[seq_num] = mmsg;
}

void Node::stopTimer(int seq_num)
{
    EV<<"Timer of seq_num = " << seq_num << " in node " << getIndex() <<" got cancelled!"<<endl;
    cancelEvent(timers[seq_num]);
    delete timers[seq_num];
    isTimerSet[seq_num] = 0;
}

void Node::sendData()
{
    MyMessage_Base * mmsg = new MyMessage_Base("");
    mmsg->setM_Payload(buffer[next_frame_to_send]);
    mmsg->setSeq_Num(next_frame_to_send);
    mmsg->setM_Type(FrameArrival);
    mmsg->setAck((frame_expected+MAX_SEQ)%(MAX_SEQ+1)); // Ack contains the number of the last frame received.
    mmsg->setTimestamp();
    // Frame with Byte Stuffing.
    frameWithByteStuffing(mmsg);
    // TODO: Hamming
    toPhysicalLayer(mmsg);
    EV<<"Network layer of Node"<<getIndex() <<" is ready and sent frame with seq_num = "<< next_frame_to_send << " and payload = " << mmsg->getM_Payload() << endl;
    startTimer(next_frame_to_send);
}

void Node::enableNetworkLayer()
{
    MyMessage_Base * mmsg = new MyMessage_Base("");
    mmsg->setM_Type(NetworkLayerReady);
    mmsg->setTimestamp();
    double interval = exponential(1 / par("lambda").doubleValue());
    scheduleAt(simTime() + interval, mmsg);
}

int Node::inc(int seq_num)
{
    return (seq_num+1) % (MAX_SEQ+1);
}

bool Node::between(int a, int b, int c){
    // return true if b is circularly in between a and b
    return (b >= a && b < c) || (b >= a && a > c) || (b < c && c < a);
}

void Node::startGoBackN(MyMessage_Base* msg)
{
    int event = msg->getM_Type();
    switch(event){
        case NetworkLayerReady:
            if(nbuffered < MAX_SEQ){
                buffer[next_frame_to_send] = fromNetworkLayer();
                ++nbuffered;
                sendData();
                next_frame_to_send = inc(next_frame_to_send);
            }
            break;
        case FrameArrival:
            // from physical layer
            if(msg->getSeq_Num() == frame_expected){
                frame_expected = inc(frame_expected);
                char* message = unframe(msg);
                EV<<"Node " << getIndex() << " just received a message with seq_num = " << msg->getSeq_Num() << " and Ack = " << msg->getAck() << " and payload = " << message << endl;
            } else {
                EV<<"Node " << getIndex() << " just received a message with seq_num = " << msg->getSeq_Num() << " and discarded it!"<<endl;

            }

            // slide window while ack received in between ack_expected and next_frame_to_send
            while(between(ack_expected, msg->getAck(), next_frame_to_send)){
                --nbuffered;
                // TODO: Stop timer of ack_expected.
                stopTimer(ack_expected);
                ack_expected = inc(ack_expected);
                EV<<"slide window of Node" << getIndex() << "[ " << ack_expected << ", " << next_frame_to_send <<" ]" <<endl;
            }
            break;
        case Timeout:
            EV << "Timeout on frame " << msg->getSeq_Num() << " in node " << getIndex()<<endl;
            next_frame_to_send = ack_expected;
            for(int i = 0; i < nbuffered; ++i){
                sendData();
                next_frame_to_send = inc(next_frame_to_send);
            }
            break;
    }
    if(nbuffered < MAX_SEQ){
        enableNetworkLayer();
    } else{
        EV<<"Network layer of Node"<<getIndex() <<" is disabled"<<endl;
    }

}

/*
 * this function make a frame using byte stuffing with :
 *
 */

void Node::frameWithByteStuffing(MyMessage_Base* mmsg){
    const char *msg = mmsg->getM_Payload();
    std::string payLoad = "";
    char flag = char(par("Flag"));
    char ESC = char(par("ESC"));
    for(int i = 0;i < (int) strlen(msg);i++){
        if(msg[i] == flag || msg[i] == ESC){
            payLoad.push_back(ESC);
            payLoad.push_back(msg[i]);
        }else{
            payLoad.push_back(msg[i]);
        }
    }
    payLoad = flag + payLoad + flag;

    char* c_payLoad = new char[payLoad.size()];
    std::strcpy(c_payLoad, payLoad.c_str());
    mmsg->setM_Payload(c_payLoad);
}

/*
 * this function make a frame using byte stuffing with :
 *
 */
char* Node::unframe(MyMessage_Base* mmsg){
   std::string msg = "";
   const char* received_msg = mmsg->getM_Payload();
   char ESC = char(par("ESC"));
   for(int i = 1 ; i < (int) strlen(received_msg) - 1; i++){
       if(received_msg[i] == ESC){
           msg.push_back(received_msg[++i]);
       }else{
           msg.push_back(received_msg[i]);
       }
   }

   char* c_msg = new char[msg.size()];
   std::strcpy(c_msg, msg.c_str());

   return c_msg;
}
//channel handling methods
/*
 * this function is used to send the message
 */
void Node::toPhysicalLayer(MyMessage_Base* msg){
    bool lost = lossEffect();
    if(!lost){
        bool dublicate = duplicateEffect(msg);
        if(dublicate){
            MyMessage_Base* mssg = msg->dup();
            modificationEffect(mssg);
            delaysEffect(mssg);
        }
        modificationEffect(msg);
        delaysEffect(msg);
    } else {
        EV<<"Frame" << msg->getSeq_Num()<< " sent from Node" << getIndex() << " is lost!!!"<<endl;
    }
}
/*
 * this function is used to simulate the modification of a bit per frame
 */
void Node::modificationEffect(MyMessage_Base* msg){
    int rand = uniform(par("modification_min").doubleValue(),par("modification_max").doubleValue())*100;
    if(par("modification_presentage").intValue()>=rand){
        int position_char = uniform(0, strlen(msg->getM_Payload()));
        const char * old_msg = msg->getM_Payload();
        int n = strlen(old_msg);
        char* m_msg = new char[n];
        std::strcpy(m_msg, old_msg);

        std::bitset<8> bits(m_msg[position_char]);

        int position_bit = uniform(0, 8);
        bits[position_bit] = ~ bits[position_bit];

        m_msg[position_char] = (char) bits.to_ulong();
        EV<<"char in position " << position_char << " changed from " << old_msg[position_char] << " to " << m_msg[position_char] << endl;
        msg->setM_Payload(m_msg);
    }
}
/*
 * this function is used to simulate the duplication effect
 */
bool Node::duplicateEffect(MyMessage_Base* msg){
    int rand = uniform(par("duplication_min").doubleValue(),par("duplication_max").doubleValue())*100;
    if(par("duplication_presentage").intValue()>=rand){
        EV<<"Frame" << msg->getSeq_Num()<< " sent from Node" << getIndex() << " is duplicated !!!"<<endl;
        return true;
    }
    return false;
}
/*
 * this function is used to simulate the loss effect of the message
 * if function returned true, then the message will be lost and won't be received
 * if false then the message will be received
 */
bool Node::lossEffect(){
    bool result = false;
    int rand = uniform(par("loss_min").doubleValue(),par("loss_max").doubleValue())*100;
    if(par("loss_presentage").intValue()>=rand){
       result = true;
    }
    return result;
}
/*
 * this function is used to simulate the delay effect of the channel by sending
 * the message after a random delay
 */
void Node::delaysEffect(MyMessage_Base* msg){

    int rand = uniform(par("delay_chance_min").doubleValue(),par("delay_chance_max").doubleValue())*100;
    if(par("delay_chance_presentage").intValue()>=rand){
        EV<<"Frame" << msg->getSeq_Num()<< " sent from Node " << getIndex() << " is delayed !!!"<<endl;
        double rand_time = uniform(par("delay_duration_min").doubleValue(),par("delay_duration_max").doubleValue());
        sendDelayed(msg,rand_time,"outs",dest);
     }else{
        send(msg, "outs", dest);
     }

}

