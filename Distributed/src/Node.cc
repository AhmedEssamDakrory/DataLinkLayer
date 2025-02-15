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


int Node::dropped_frames = 0;
int Node::generated_frames = 0;
int Node::retransmitted_frames = 0;
int Node::duplicated_frames = 0;
int Node::useful_frames = 0;
bool Node::printed = false;

void Node::initialize()
{

    // Read from the text file
//    std::string filename = "node" + std::to_string(getIndex()+1) + ".txt";
    std::string filename = "file.txt";
    std::ifstream MyReadFile(filename);
    EV << "Reading messages from " << filename << endl;

    // Use a while loop together with the getline() function to read the file line by line
    std::string myText;
    while (getline (MyReadFile, myText)) {
        msgs.push_back(myText);
    }
    msgs_count = msgs.size();

    EV << msgs_count << " Messages read successfully" << endl;

    // Close the file
    MyReadFile.close();

    sent_frames = 0;
}



void Node::handleMessage(cMessage *msg)
{
    // end time
    if(simTime() >= par("end_time")){
        if(!printed && getIndex() == 0){
            printStatistics();
            printed = true;
        }
        return;
    }

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
    int idx = sent_frames++%int(msgs_count);
    const char* msg = msgs[idx].c_str();
    return msg;
}


void Node::startTimer(int seq_num)
{
    MyMessage_Base * mmsg = new MyMessage_Base("");
    mmsg->setM_Type(Timeout);
    mmsg->setSeq_Num(seq_num);
    mmsg->setTimestamp();
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

void Node::sendData(bool retransmitted)
{
    MyMessage_Base * mmsg = new MyMessage_Base("");
    mmsg->setM_Payload(buffer[next_frame_to_send]);
    mmsg->setSeq_Num(next_frame_to_send);
    mmsg->setM_Type(FrameArrival);
    mmsg->setAck((frame_expected+MAX_SEQ)%(MAX_SEQ+1)); // Ack contains the number of the last frame received.
    mmsg->setTimestamp();
    // Frame with Byte Stuffing.
    EV<<"Network layer of Node"<<getIndex() <<" is ready and sent frame with seq_num = "<< next_frame_to_send << " and payload = " << mmsg->getM_Payload() << endl;
    frameWithByteStuffing(mmsg);
    EV << "Payload after framing: " << mmsg->getM_Payload() << endl;
    // Hamming
    addHamming(mmsg);
    EV << "Payload after hamming code: " << mmsg ->getM_Payload() << endl;
    toPhysicalLayer(mmsg, retransmitted);
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
                sendData(false);
                next_frame_to_send = inc(next_frame_to_send);
            }
            break;
        case FrameArrival:
            // from physical layer
            if(msg->getSeq_Num() == frame_expected){
                frame_expected = inc(frame_expected);
                EV<<"Node " << getIndex() << " just received a message with seq_num = " << msg->getSeq_Num() << " and Ack = " << msg->getAck() << " and payload = " << msg->getM_Payload() << endl;
                correctErrors(msg);
                EV<<"Payload after error correction and removal of hamming codes: " << msg->getM_Payload() << endl;
                char* message = unframe(msg);
                EV<<"Payload after removal of framing (original message): " << message << endl;
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
                sendData(true);
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
void Node::toPhysicalLayer(MyMessage_Base* msg, bool retransmitted){
    generated_frames++;
    bool lost = lossEffect();
    if(retransmitted) retransmitted_frames++;
    if(!lost){
        if(!retransmitted) useful_frames++;
        bool dublicate = duplicateEffect(msg);
        if(dublicate){
            generated_frames++;
            duplicated_frames++;
            MyMessage_Base* mssg = msg->dup();
            modificationEffect(mssg);
            delaysEffect(mssg);
        }
        modificationEffect(msg);
        delaysEffect(msg);
    } else {
        dropped_frames++;
        EV<<"Frame" << msg->getSeq_Num()<< " sent from Node" << getIndex() << " is lost!!!"<<endl;
    }
}
/*
 * this function is used to simulate the modification of a bit per frame
 */
void Node::modificationEffect(MyMessage_Base* msg){
    int rand = uniform(par("modification_min").doubleValue(),par("modification_max").doubleValue())*100;
    if(par("modification_presentage").intValue()>=rand){
        const char * old_msg = msg->getM_Payload();
        int n = strlen(old_msg);
        int position_char = uniform(0, n);
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

bool Node::isPowerOfTwo(int x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

std::string Node::binarize(std::string s) {
    std::string out = "";
    for(int i=0;i<int(s.length());i++) {
        out += std::bitset<8>(s[i]).to_string();
    }
    return out;
}

std::string Node::characterize(std::string s) {
    std::string out = "";
    while(s.length()%8!=0){
        s+='0';
    }
    for(int i=0;i<int(s.length()/8);i++) {
        out += static_cast<char>(std::bitset<8>(s.substr(i*8, 8)).to_ulong());
    }
    return out;
}

void Node::addHamming(MyMessage_Base *mmsg){
    std::string msg = mmsg->getM_Payload();
    msg = binarize(msg);
    int m = msg.length();

    // calculating r
    int r = floor(log2(m+1))+1;
    while(pow(2,r) <= m+r+1) r++;

    // initializing outputMsg
    std::string outputMsg = "";
    int k=0;
    for(int i=1; i<=m+r; i++) {
        if(!isPowerOfTwo(i)) outputMsg+=msg[k++];
        else outputMsg+=" ";
    }

    // calculating parity bits
    for(int i=0; i<r; i++) {
        int p = 0;
        for(int j=1; j<=m+r; j++) {
            if(!isPowerOfTwo(j) && (j&(int)pow(2, i))) {
                p^=(outputMsg[j-1]&1);
            }
        }
        outputMsg[pow(2, i)-1] = p+48;
    }
    outputMsg = characterize(outputMsg);
    mmsg->setM_Payload(outputMsg.c_str());
}


void Node::correctErrors(MyMessage_Base *mmsg){
    std::string msg = mmsg->getM_Payload();
    int n = msg.length()*8;
    int r = ceil(log2(n));
    int m = ((n-r)/8)*8;
    msg = binarize(msg).substr(0, m+r);

    int error = 0;
    for(int i=0; i<r; i++) {
        int p = 0;
        for(int j=1; j<=n; j++) {
            if(!isPowerOfTwo(j) && (j&(int)pow(2, i))) {
                p^=(msg[j-1]&1);
            }
        }
        if((char)p+48 != msg[pow(2, i)-1]) {
            error |= (int)pow(2, i);
        }
    }
    if(error) {
        EV << "Node " << getIndex() << ": An Error was detected and corrected at bit #" << error << endl;
        int e = msg[error-1] - '0';
        if(e) msg[error-1] = '0';
        else msg[error-1] = '1';
    }
    std::string correctedMsg = "";
    for(int i=1; i<=m+r; i++) {
        if(!isPowerOfTwo(i)) correctedMsg+=msg[i-1];
    }

    correctedMsg = characterize(correctedMsg);
    mmsg->setM_Payload(correctedMsg.c_str());
}

void Node::printStatistics(){
    std::cout<<"Number of generated frames: " << generated_frames << endl;
    std::cout<<"Number of dropped frames: " << dropped_frames << endl;
    std::cout<<"Number of retransmitted frames: " << retransmitted_frames << endl;
    std::cout<<"Number of duplicated frames: " << duplicated_frames << endl;
    std::cout<<"percentage of useful data: " << (double(useful_frames)/ double(generated_frames))*100 << endl;
}

