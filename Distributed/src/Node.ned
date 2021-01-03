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

package distributed;

//
// TODO auto-generated module
//
simple Node
{
    parameters:
        double lambda = default(2.0);
        double timeout_period = default(5);
        int m = default(2); // number of bits needed for the sequence number.  
        
        //framing parameters 
        int flag = default(36); // acsci of $    
        int ESC = default(47); // asci of /
        
        //channel parameters    

        double modification_min = default(0.0);
        double modification_max = default(1.0);
        int modification_presentage = default(30);
        
        double loss_min = default(0.0);
        double loss_max = default(1.0);
        int loss_presentage = default(30);
        
        double duplication_min = default(0.0);
        double duplication_max = default(1.0);
        int duplication_presentage = default(20);
        
        double delay_chance_min = default(0.0);
        double delay_chance_max = default(1.0);
        int delay_chance_presentage = default(20);
        
        double delay_duration_min = default(0.0);
        double delay_duration_max = default(1.0);
        
    gates:
        input ins[];
        output outs[];
}
