#include "scene/pose.hpp"
#include <cmath>
#include <cstdio>
namespace { constexpr float pi=3.14159265358979323846f; bool near(float a,float b,float e=1e-4f){return std::fabs(a-b)<=e;}
void rot(cr::NodePose& p,float d){float a=d*pi/180,c=std::cos(a),s=std::sin(a);p.cols[0][0]=c;p.cols[0][1]=s;p.cols[0][2]=0;p.cols[1][0]=-s;p.cols[1][1]=c;p.cols[1][2]=0;p.cols[2][0]=0;p.cols[2][1]=0;p.cols[2][2]=1;}
float angle(const cr::NodePose& p){return std::atan2(p.cols[0][1],p.cols[0][0])*180/pi;} }
int main(){int bad=0;cr::NodePose a{},b{};rot(a,0);rot(b,90);a.s[0]=a.s[1]=a.s[2]=1;b.s[0]=b.s[1]=b.s[2]=3;a.t[0]=2;b.t[0]=6;std::vector<cr::NodePose> out;
cr::blendLocalPoses({a},{b},0,out);bad+=!near(angle(out[0]),0);cr::blendLocalPoses({a},{b},1,out);bad+=!near(angle(out[0]),90);
cr::blendLocalPoses({a},{b},.5f,out);bad+=!near(angle(out[0]),45,.01f)||!near(out[0].t[0],4)||!near(out[0].s[0],2);
float dot=out[0].cols[0][0]*out[0].cols[1][0]+out[0].cols[0][1]*out[0].cols[1][1]+out[0].cols[0][2]*out[0].cols[1][2];bad+=!near(dot,0);
rot(a,350);rot(b,10);cr::blendLocalPoses({a},{b},.5f,out);bad+=std::fabs(angle(out[0]))>.01f;
cr::Skeleton sk;sk.nodes.resize(1);std::vector<cr::Mat4>w;cr::computeWorldFromLocalPoses(sk,out,w);bad+=w.size()!=1||!near(w[0].m[15],1);
std::printf("pose blend: endpoints, midpoint, shortest arc, translation, scale, world resolve; %d problems\n",bad);return bad?1:0;}