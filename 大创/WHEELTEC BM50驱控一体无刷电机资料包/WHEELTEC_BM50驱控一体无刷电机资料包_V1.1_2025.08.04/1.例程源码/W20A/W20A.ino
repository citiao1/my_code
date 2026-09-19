#include <MsTimer2.h>        //定时中断

#define M_DIR         7   //方向控制引脚       
#define M_PWM         6   //电机PWM信号输入引脚
#define M_BTAK        5   //电机刹车引脚              
#define M_EncoderB    4   //编码器采集引脚
#define M_EncoderA    2   //编码器采集引脚

#define u32           unsigned int

int Target_Encoder=4, Current_Encoder;
volatile long Encoder_Num = 0;
int Test_cnt;
float Velocity_KP = 1.5, Velocity_KI = 0.5;
int Motor_Pwm=0;

int Incremental_PI(int Encoder,int Target)
{ 	
	 static int Bias,Pwm,Last_bias;
	 Bias=Target-Encoder;                					//计算偏差
	 Pwm+=Velocity_KP*(Bias-Last_bias)+Velocity_KI*Bias;   	//增量式PI控制器
	 Last_bias=Bias;	                   					//保存上一次偏差 
	 return Pwm;                         					//增量输出
}

float PWM_Limit(float IN,int max,int min)
{
	float OUT = IN;
	if(OUT>max) OUT = max;
	if(OUT<min) OUT = min;
	return OUT;
}

u32 myabs(long a)
{ 		   
	  u32 temp;
		if(a<0)  temp=-a;  
	  else temp=a;
	  return temp;
}

void Set_PWM(int motorPwm){
  if(motorPwm < 0)    digitalWrite(M_DIR, LOW);
  else                digitalWrite(M_DIR, HIGH);
  analogWrite(M_PWM, myabs(Motor_Pwm));
}

void control(void){
  Current_Encoder = Encoder_Num;  Encoder_Num = 0;
  if(++Test_cnt == 500){
    Test_cnt = 0;
    Target_Encoder = -Target_Encoder;
  }
  Motor_Pwm = Incremental_PI(Current_Encoder, Target_Encoder);
  Motor_Pwm = PWM_Limit(Motor_Pwm, 254, -254);
  Set_PWM(Motor_Pwm);
}

void READ_ENCODER(void){
  if (digitalRead(M_EncoderA) == LOW) {     //如果是下降沿触发的中断
    if (digitalRead(M_EncoderB) == LOW)      Encoder_Num--;  //根据另外一相电平判定方向
    else      Encoder_Num++;
  }
  else {     //如果是上升沿触发的中断
    if (digitalRead(M_EncoderB) == LOW)      Encoder_Num++; //根据另外一相电平判定方向
    else     Encoder_Num--;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(M_DIR, OUTPUT);
  pinMode(M_PWM, OUTPUT);
  TCCR0A = (TCCR0A & 0b00111111) | (0b11 << COM0A0);    //将6号引脚修改为低电平有效，只修改COM0A位，保留其他位（包括WGM和COM0B）
  pinMode(M_BTAK, OUTPUT);
  pinMode(M_EncoderA, INPUT);
  pinMode(M_EncoderB, INPUT);
  digitalWrite(M_DIR, HIGH);      //
  // analogWrite(M_PWM, 15);        //15/255,占空比四舍五入等于6%，因为上面修改为低电平有效，所以这里占空比代表的是低电平在一个周期的占比
  digitalWrite(M_BTAK, HIGH);     //刹车引脚，低电平使能，也就是低电平刹车，高电平正常转动
  MsTimer2::set(10, control);     //3、11号引脚使用的是定时器2输出PWM信号，所以想要输出PWM信号不能使用3、11号引脚，或者说不能使用定时器2的引脚
  MsTimer2::start();
  attachInterrupt(digitalPinToInterrupt(M_EncoderA), READ_ENCODER, CHANGE);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Current = ");
  Serial.print(Current_Encoder);
  Serial.print("     ");
  Serial.print("Target = ");
  Serial.print(Target_Encoder);
  Serial.print("     ");
  Serial.print("Motor_Pwm = ");
  Serial.println(Motor_Pwm);
  delay(100);
}
