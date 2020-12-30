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
Define_Module(Node);

void Node::initialize()
{
    initGoBackN();
    enableNetworkLayer();
}



void Node::handleMessage(cMessage *msg)
{
    MyMessage_Base *mmsg = check_and_cast<MyMessage_Base *>(msg);
    startGoBackN(mmsg);
}

int Node::getDest()
{
    int dest = getIndex();
    if(dest % 2 != 0) --dest;
    return dest;
}

void Node::initGoBackN()
{
    next_frame_to_send = 0;
    ack_expected = 0;
    frame_expected = 0;
    nbuffered = 0;
    int m = par("m");
    MAX_SEQ = (1 << m) -1; // window size should be less than (2^m)
    buffer = new char*[MAX_SEQ+1];
}

char* Node::fromNetworkLayer()
{
    // dummy TODO:return random message
    char* message = "Hello there";
    return  message;
}

void Node::sendData()
{
    // TODO: Framing and Hamming code
    MyMessage_Base * mmsg = new MyMessage_Base("");
    mmsg->setM_Payload(buffer[next_frame_to_send]);
    mmsg->setSeq_Num(next_frame_to_send);
    mmsg->setM_Type(FrameArrival);
    mmsg->setAck((frame_expected+MAX_SEQ)%(MAX_SEQ+1)); // Ack contains the number of the last frame received.
    //send(mmsg, "outs", getDest()); // to physical layer
    toPhysicalLayer(mmsg);
    //TODO: start timer.
}

void Node::enableNetworkLayer()
{
    // TODO: generate event with probability

    MyMessage_Base * mmsg = new MyMessage_Base("");
    mmsg->setM_Type(NetworkLayerReady);
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
            EV<<"Network layer of Node"<<getIndex() <<" is ready and sent frame with seq_num = "<< next_frame_to_send << endl;
            buffer[next_frame_to_send] = fromNetworkLayer();
            ++nbuffered;
            sendData();
            next_frame_to_send = inc(next_frame_to_send);
            break;
        case FrameArrival:
            // from physical layer
            if(msg->getSeq_Num() == frame_expected){
                frame_expected = inc(frame_expected);
                EV<<"Node" << getIndex() << " just received a message with seq_num = " << msg->getSeq_Num() << " and Ack = " << msg->getAck()<<endl;
            }

            // slide window while ack received in between ack_expected and next_frame_to_send
            std::cout<<msg->getAck()<<endl;
            while(between(ack_expected, msg->getAck(), next_frame_to_send)){
                EV<<"slide window of Node" << getIndex() << "[ " << ack_expected << ", " << next_frame_to_send <<" ]" <<endl;
                --nbuffered;
                // TODO: Stop timer of ack_expected.
                ack_expected = inc(ack_expected);
            }
            break;
        case Timeout:
            next_frame_to_send = ack_expected;
            for(int i = 0; i < nbuffered; ++i){
                sendData();
                inc(next_frame_to_send);
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

MyMessage_Base Node::frame(char *msg ){
    MyMessage_Base mmsg("") ;
    std::string payLoad = "";
    const char* flag = par("Flag").stringValue();
    const char* ESC = par("ESC").stringValue();
    for(int i = 0;i<strlen(msg);i++){
        if(msg[i] == *flag || msg[i] == *ESC){
            payLoad.push_back(par("ESC"));
            payLoad.push_back(msg[i]);
        }else{
            payLoad.push_back(msg[i]);
        }
    }
    payLoad = "$"+payLoad+"$";

    char* c_payLoad = new char[payLoad.size()];
    std::strcpy(c_payLoad, payLoad.c_str());
    mmsg.setM_Payload(c_payLoad);
    return mmsg;
}

/*
 * this function make a frame using byte stuffing with :
 *
 */
char* Node::unframe(MyMessage_Base mmsg){
   std::string msg = "";
   const char* received_msg = mmsg.getM_Payload();
   const char* ESC = par("ESC").stringValue();
   for(int i = 1 ; i < strlen(received_msg) - 1; i++){
       if(received_msg[i] == *ESC){
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
        duplicateEffect(msg);
        modificationEffect(msg);
        delaysEffect(msg);
    }
}
/*
 * this function is used to simulate the modification of a bit per frame
 */
void Node::modificationEffect(MyMessage_Base* msg){
    int rand = uniform(par("modification_min").doubleValue(),par("modification_max").doubleValue())*100;
    if(par("modification_presentage").intValue()>=rand){
        int position_bit = uniform(0,(strlen(msg->getM_Payload())*8) - 1);
        int position_char = position_bit / 8;
        const char * old_msg = msg->getM_Payload();
        int n = strlen(old_msg);
        char* m_msg =new char[n];
        std::strcpy(m_msg, old_msg);

        std::bitset<8> bits(m_msg[position_char]);
        bits[position_bit] = ~bits[position_bit];

        m_msg[position_char] = *(bits.to_string().c_str());
        msg->setM_Payload(m_msg);
        EV<<"frame is modified at bit position : "<< position_bit <<endl;
        EV<<"frame is modified at char position : "<< position_char <<endl;
        EV<<old_msg[position_char]<< " --> " <<m_msg[position_char]<<endl;

    }



}
/*
 * this function is used to simulate the duplication effect
 */
void Node::duplicateEffect(MyMessage_Base* msg){
    int rand = uniform(par("duplication_min").doubleValue(),par("duplication_max").doubleValue())*100;
    if(par("duplication_presentage").intValue()>=rand){
       EV<<"frame is duplicated !!!"<<endl;
       const char * old_msg = msg->getM_Payload();
       int n = 2*strlen(old_msg);
       char* d_msg =new char[n];
       for(int i = 0 ; i < n;i++){
           d_msg[i] = old_msg[i%(n/2)];
       }
       msg->setM_Payload(d_msg);
    }
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
       EV<<"frame is lost !!!"<<endl;
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
        EV<<"frame is delayed !!!"<<endl;
        double rand_time = uniform(par("delay_duration_min").doubleValue(),par("delay_duration_max").doubleValue());
        sendDelayed(msg,rand_time,"out",getDest());
     }else{
        send(msg, "outs", getDest());
     }

}

