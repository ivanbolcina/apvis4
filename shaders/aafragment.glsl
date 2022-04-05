uniform sampler2D texture;
varying vec4 texc;
uniform int texturePresent = 1;

void main(void)
{
    if (texturePresent==0) {gl_FragColor=vec4(0.2f,0.2f,0.2f,1); return;}
   // vec4 texc=gl_FragCoord;
    if (abs(texc.st.x)<0.1 || abs(texc.st.x)>0.9  || abs(texc.st.y)>0.9  || abs(texc.st.y)<0.1 ) 
    {
        //angle
        float angle=atan(texc.y-0.5,texc.x-0.5);
        //indexes
        int i=0;
        int j=0;
        //colorsum
        vec4 sum=vec4(0,0,0,0); 
        float sumw=0;
        //center
        vec2 got;
        for (i=0;i<10;i++){
            for (j=0;j<10;j++){
              //weight
              float w=5-abs((float)(i-5))+5-abs((float)(j-5));
              //angle point
              float angle2=(float)(i-5);
              angle2=angle2/200.0;
              angle2=angle2+angle;
              //vector 
              float len2=j-5;
              len2=len2/200.0;              
              float mullen=1.0+abs(sin(2*angle2))*0.45;
              len2=len2+mullen*0.4;              
              //final position
              vec2 pos=vec2(0.5,0.5)+vec2(cos(angle2)*len2,sin(angle2)*len2);
              //is it center
              if (i==5 && j==5) got=pos;
              //add to sum
              sumw=sumw+w; 
              sum = sum+ texture2D(texture, pos)*w;
            }
        }
        //de-weight
        sum=sum/sumw;
 
        //calculate intenzitivity based on border distance
        float x1=texc.st.x;
        if (x1>0.1 && x1<=0.9) x1=0.1;
        if (x1>0.9) x1=1.0-x1;
        float y1=texc.st.y;
        if (y1>0.1 && y1<=0.9) y1=0.1; 
        if (y1>0.9) y1=1.0-y1;
        x1=x1*10.0;
        y1=y1*10.0;
        //distance to border known, calculate intenzitivity now
        float len=0;
        len=x1*y1;
        len=sqrt(len*len*len);
        //some experiments
        //len=len*len;
        //len=sqrt(len);
        //x1=x1*x1;
        //y1=y1*y1;
        //float len=min(x1,y1);
        //float angle3=min(atan(x1*x1,y1*y1),atan(y1*y1,x1*x1));
        //float mullen2=1.0+abs(sin(2*angle3))*0.45;
        //float dist=sqrt(x1*x1+y1*y1)/mullen2;
        //dist=dist*dist;
        //len=dist;
        //len=len*len;
        //add intenzitivity to sum
        sum=sum*len;
        
        //add background
        sum.r=sum.r+0.2;
        sum.g=sum.g+0.2;
        sum.b=sum.b+0.2;
        //save result
       // gl_FragColor=sum;
        sum.a=1;
        gl_FragColor=sum;
    }
    else
    {
        vec2 v_new = vec2(1.25*(texc.x - 0.1),1.25*(texc.y - 0.1));
       // vec2 v_new = texc; 
        gl_FragColor = texture2D(texture, v_new);
    }
    //if (texture==0) glFragColor=vec4(0,0,0,0);

     float x=texc[0];
     float y=texc[1];

//        gl_FragColor=vec4(x*y,x*y,x*y,1);
}
