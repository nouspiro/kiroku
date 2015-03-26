#Wr = 0.299;
#Wb = 0.114;

Wr = 0.2126;
Wb = 0.0722;
Wg = 1-Wr-Wb;

Umax = 0.436;
Vmax = 0.615;

RGB2YUV = [
	Wr, Wg, Wb, 0;
	-Umax*Wr/(1-Wb), -Umax*Wg/(1-Wb), (Umax-Umax*Wb)/(1-Wb), 0;
	(Vmax-Vmax*Wr)/(1-Wr), -Vmax*Wg/(1-Wr), -Vmax*Wb/(1-Wr), 0;
	0, 0, 0, 1
];

# 16 
# 235
# 240

Yc = (235-16)/255;
Cc = (240-16)/255;
d  = 16/255;

S = [
	Yc, 0, 0, d;
	0, Cc/(2*Umax), 0, Cc/2+d;
	0, 0, Cc/(2*Vmax), Cc/2+d
];

S*RGB2YUV
