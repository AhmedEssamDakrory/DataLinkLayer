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
    send(mmsg, "outs", getDest()); // to physical layer
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

