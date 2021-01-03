package distributed;

@license(LGPL);

import ned.IdealChannel;

network Distributed
{
    parameters:
        int n = default(5);
        @display("bgb=374,222");
    submodules:
        nodes[n]: Node;
    connections:
        for i=0..sizeof(nodes)-1, for j=i+1..sizeof(nodes)-1 {
            nodes[i].outs++ --> IdealChannel --> nodes[j].ins++;
            nodes[i].ins++ <-- IdealChannel <-- nodes[j].outs++;
        }
}
