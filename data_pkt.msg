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
packet DataPkt{
	//以Flit的形式来表现
	
	//以下为head flit具有的信息
	int flitCount;//如果为head flit，则该项记录包括head flit在内的flit个数
	int src_ppid; //源processor的physical id
	int dst_ppid; //目标processor的physical id
	long packageGenTime; //package的产生时间
       
    //以下为每个flit都具有的信息
	bool isHead; //判断是否为head flit
	bool isTail; //判断是否为tail flit
	int vc_id; //virtual channel id
	
	int hopCount = 0;
	int from_router_port; //记录从当前路由器的哪一个端口收到该msg，由上一个路由器计算出
	
	//debug message
	double transmit_start;
	
	
}