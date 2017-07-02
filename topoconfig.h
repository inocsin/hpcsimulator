/*
 * topoconfig.h
 *
 *  Created on: 2017��4��25��
 *      Author: Vincent
 */

#ifndef TOPOCONFIG_H_
#define TOPOCONFIG_H_


/*
//tianhe_router
//�������˲���
#define PortNum 24
#define ProcessorNum 24

//#define LinkNum 48 //˫����·����2�����ڹ��ķ���
//���5000��ز�����16�˿ڣ�3��ͨ����MTU 1KB��Buffer 4MTU

#define PacketSize 128 //ByteΪ��λ
#define FlitSize 4 //ByteΪ��λ
#define FlitLength 32 // PacketSize / FlitSize�� FlitLength������ڵ���2������processor���������ݰ��ᷢ������

#define VC 3 //virtual channel
#define BufferDepth 4 * FlitLength //virtual channel buffer depth, ���ڵ���2���ɴ��Flit������
#define ProcessorBufferDepth 4 * FlitLength // processor txQueue �Ĵ�С

//ʱ����ز���
#define FREQ 3500000000.0  //���ڹ��ķ��棬��λhz

*/

#define PortNum 24
#define ProcessorNum 24
#define PacketSize 128
#define FlitSize 4
#define FlitLength 32
#define VC 3
#define BufferDepth 4 * FlitLength
#define ProcessorBufferDepth 4 * FlitLength
#define FREQ 3500000000.0
#define OutBufferDepth 29

//******************�����ֹ��޸ģ������Զ�����*******************8

#define CLK_CYCLE 1/FREQ //ʱ������
#define Sim_Start_Time 1 //1s ��ʼ����
#define TimeScale 0.1 //���ģ����ڲ��ɷֲ��������Ʒֲ�����lambda=10����ʾ1s��10��flit���õ���ʱ��������TimeScale��roundȡ��

//Spatial Distribution
#define UNIFORM //�ռ���ȷֲ�

//Time Distribution
//#define SELF_SIMILARITY //�����Ʒֲ�
//#define POISSON_DIST //���ò��ɷֲ�

//�����ƺͲ��ɷֲ���ȡֵ��ΧС��10����λʱ��Ϊ1s����TimeScale����л��㣬������ʱ������
//�����ƺͲ��ɷֲ��Ĳ���

//�����Ʒֲ�Pareto����
//#define ALPHA_ON 4
//#define ALPHA_OFF 2

//Poisson�ֲ�����
#define LAMBDA 7 //���ɷֲ������ڲ���ʱ������ָ���ֲ���lambda����ʾ��λʱ����(1s)�����֡�����䵹��Ϊʱ������ƽ��ֵ
//Uniform�ֲ�����
#define INJECTION_RATE 1.8 //ע���ʣ���Χ��0.1 - 1֮��

//������Ϣ
#define Verbose 1
#define VERBOSE_DEBUG_MESSAGES 1
#define VERBOSE_DETAIL_DEBUG_MESSAGES 2



//������ز���
//����ܹ��ղ���
#define LVT 1
#define NVT 2
#define HVT 3

//#define TR 0.2//toggle rate
#define VDD 1.0
#define PARM(x) PARM_ ## x
#define PARM_TECH_POINT 45
#define PARM_TRANSISTOR_TYPE NVT
#define FlitWidth FlitSize * 8



#endif /* TOPOCONFIG_H_ */
