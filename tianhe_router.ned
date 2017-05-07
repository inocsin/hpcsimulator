simple SimpleRouter
{
	parameters:

	gates:
		inout port_0;
		inout port_1;
		inout port_2;
		inout port_3;
		inout port_4;
		inout port_5;
		inout port_6;
		inout port_7;
		inout port_8;
		inout port_9;
		inout port_10;
		inout port_11;
		inout port_12;
		inout port_13;
		inout port_14;
		inout port_15;
		inout port_16;
		inout port_17;
		inout port_18;
		inout port_19;
		inout port_20;
		inout port_21;
		inout port_22;
		inout port_23;
}
simple SimpleProcessor
{
	parameters:

	gates:
		inout port;
}
network Tianhe_Router
{
	types:
		channel Channel extends ned.DatarateChannel
		{
			delay = 0ns;
			datarate = 112Gbps;
		}
	submodules:
		router[1]: SimpleRouter {

		}
		processor[24]: SimpleProcessor{

		}
	connections:
		processor[0].port <--> Channel <--> router[0].port_0;
		processor[1].port <--> Channel <--> router[0].port_1;
		processor[2].port <--> Channel <--> router[0].port_2;
		processor[3].port <--> Channel <--> router[0].port_3;
		processor[4].port <--> Channel <--> router[0].port_4;
		processor[5].port <--> Channel <--> router[0].port_5;
		processor[6].port <--> Channel <--> router[0].port_6;
		processor[7].port <--> Channel <--> router[0].port_7;
		processor[8].port <--> Channel <--> router[0].port_8;
		processor[9].port <--> Channel <--> router[0].port_9;
		processor[10].port <--> Channel <--> router[0].port_10;
		processor[11].port <--> Channel <--> router[0].port_11;
		processor[12].port <--> Channel <--> router[0].port_12;
		processor[13].port <--> Channel <--> router[0].port_13;
		processor[14].port <--> Channel <--> router[0].port_14;
		processor[15].port <--> Channel <--> router[0].port_15;
		processor[16].port <--> Channel <--> router[0].port_16;
		processor[17].port <--> Channel <--> router[0].port_17;
		processor[18].port <--> Channel <--> router[0].port_18;
		processor[19].port <--> Channel <--> router[0].port_19;
		processor[20].port <--> Channel <--> router[0].port_20;
		processor[21].port <--> Channel <--> router[0].port_21;
		processor[22].port <--> Channel <--> router[0].port_22;
		processor[23].port <--> Channel <--> router[0].port_23;
}
