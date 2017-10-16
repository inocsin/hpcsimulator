simple FtRouter
{
	parameters:

	gates:
		inout port_0;
		inout port_1;
		inout port_2;
		inout port_3;
}
simple FtProcessor
{
	parameters:

	gates:
		inout port;
}
network Fat_tree
{
	types:
		channel Channel extends ned.DatarateChannel
		{
			delay = 5.0ns;
//			datarate = 112Gbps;
		}
	submodules:
		router[20]: FtRouter {

		}
		processor[16]: FtProcessor{

		}
	connections:
		processor[0].port <--> Channel <--> router[0].port_0;
		processor[1].port <--> Channel <--> router[0].port_1;
		processor[2].port <--> Channel <--> router[1].port_0;
		processor[3].port <--> Channel <--> router[1].port_1;
		processor[4].port <--> Channel <--> router[2].port_0;
		processor[5].port <--> Channel <--> router[2].port_1;
		processor[6].port <--> Channel <--> router[3].port_0;
		processor[7].port <--> Channel <--> router[3].port_1;
		processor[8].port <--> Channel <--> router[4].port_0;
		processor[9].port <--> Channel <--> router[4].port_1;
		processor[10].port <--> Channel <--> router[5].port_0;
		processor[11].port <--> Channel <--> router[5].port_1;
		processor[12].port <--> Channel <--> router[6].port_0;
		processor[13].port <--> Channel <--> router[6].port_1;
		processor[14].port <--> Channel <--> router[7].port_0;
		processor[15].port <--> Channel <--> router[7].port_1;
		router[0].port_2 <--> Channel <--> router[8].port_0;
		router[0].port_3 <--> Channel <--> router[9].port_0;
		router[1].port_2 <--> Channel <--> router[8].port_1;
		router[1].port_3 <--> Channel <--> router[9].port_1;
		router[2].port_2 <--> Channel <--> router[10].port_0;
		router[2].port_3 <--> Channel <--> router[11].port_0;
		router[3].port_2 <--> Channel <--> router[10].port_1;
		router[3].port_3 <--> Channel <--> router[11].port_1;
		router[4].port_2 <--> Channel <--> router[12].port_0;
		router[4].port_3 <--> Channel <--> router[13].port_0;
		router[5].port_2 <--> Channel <--> router[12].port_1;
		router[5].port_3 <--> Channel <--> router[13].port_1;
		router[6].port_2 <--> Channel <--> router[14].port_0;
		router[6].port_3 <--> Channel <--> router[15].port_0;
		router[7].port_2 <--> Channel <--> router[14].port_1;
		router[7].port_3 <--> Channel <--> router[15].port_1;
		router[8].port_2 <--> Channel <--> router[16].port_0;
		router[8].port_3 <--> Channel <--> router[18].port_0;
		router[9].port_2 <--> Channel <--> router[17].port_0;
		router[9].port_3 <--> Channel <--> router[19].port_0;
		router[10].port_2 <--> Channel <--> router[16].port_1;
		router[10].port_3 <--> Channel <--> router[18].port_1;
		router[11].port_2 <--> Channel <--> router[17].port_1;
		router[11].port_3 <--> Channel <--> router[19].port_1;
		router[12].port_2 <--> Channel <--> router[16].port_2;
		router[12].port_3 <--> Channel <--> router[18].port_2;
		router[13].port_2 <--> Channel <--> router[17].port_2;
		router[13].port_3 <--> Channel <--> router[19].port_2;
		router[14].port_2 <--> Channel <--> router[16].port_3;
		router[14].port_3 <--> Channel <--> router[18].port_3;
		router[15].port_2 <--> Channel <--> router[17].port_3;
		router[15].port_3 <--> Channel <--> router[19].port_3;
}
