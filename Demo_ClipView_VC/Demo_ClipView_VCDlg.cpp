
// Demo_ClipView_VCDlg.cpp : 实现文件
//
#include "stdafx.h"
#include "Demo_ClipView_VC.h"
#include "Demo_ClipView_VCDlg.h"
#include "afxdialogex.h"
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PI 3.141592653
#define max(a,b) ((a>b)?a:b)
#define min(a,b) ((a<b)?a:b)

// CDemo_ClipView_VCDlg 对话框

#define CANVAS_WIDTH	800
#define CANVAS_HEIGHT	600
#define INFO_HEIGHT		50
#define TESTDATA_XML1  "TestData1.xml"
#define TESTDATA_XML2  "TestData2.xml"

//裁剪算法相关定义
const int INTIALIZE=0;
const double ESP=1e-5;




///////////////////////////////////////
// main part begins


void CDemo_ClipView_VCDlg::OnBnClickedBtnClip()
{
	//以下为暂时的绘图代码
	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));

	COLORREF clrBoundary = RGB(255,0,0);
	DrawBoundary(boundary, clrBoundary);

	COLORREF clrLine = RGB(0,255,0);
	//以上为暂时的绘图代码


	BeginTimeAndMemoryMonitor();

	if (boundary.isConvex)
		dealConvex();
	else
		dealConcave();

	forCircleRun();

	EndTimeAndMemoryMonitor();
}

// main part ends
/////////////////////////////////////////





//Tip: You can use Ctrl+F to search one of the following words to get to your codes quickly:   GS   XH   DQC


//////////////////////////////////
// GS's codes begin


//判断线段是否在包围盒里
bool InBox(Boundary & boundary,Line line){
	bool isVisible=true;
	int xl=INTIALIZE,xr=-INTIALIZE,yt=-INTIALIZE,yb=INTIALIZE;
	int n=boundary.vertexs.size();
	for(int i=0;i<n-1;i++){
		if(boundary.vertexs[i].x<=xl) xl=boundary.vertexs[i].x;
		if(boundary.vertexs[i].x>=xr) xr=boundary.vertexs[i].x;
		if(boundary.vertexs[i].y>=yt) yt=boundary.vertexs[i].y;
		if(boundary.vertexs[i].y<=yb) yb=boundary.vertexs[i].y;
	}
	if(line.startpoint.x<xl&&line.endpoint.x<xl) return false;
	else if((line.startpoint.x>xr)&&(line.endpoint.x>xr)) return false;
	else if(line.startpoint.y>yt&&line.endpoint.y>yt) return false;
	else if(line.startpoint.y<yb&&line.endpoint.y<yb) return false;
	else return true;
}

//过点(x1,y1)(x2,y2)一般式直线方程 (y2-y1)x+(x1-x2)y+x2y1-x1y2=0
int Multinomial(int x1,int y1,int x2,int y2,int x,int y){
	int result=(y2-y1)*x+(x1-x2)*y+x2*y1-x1*y2;
	return result;
}

//求两条直线的交点
CPoint CrossPoint(Line line1,Line line2){
	CPoint CrossP;
	int a1 = line1.endpoint.y-line1.startpoint.y;
	int b1 = line1.startpoint.x-line1.endpoint.x;
	int c1 = line1.endpoint.x*line1.startpoint.y-line1.startpoint.x*line1.endpoint.y;

    int a2 = line2.endpoint.y-line2.startpoint.y;
	int b2 = line2.startpoint.x-line2.endpoint.x;
	int c2 = line2.endpoint.x*line2.startpoint.y-line2.startpoint.x*line2.endpoint.y;

	if((b1*a2-a1*b2)!=0){
		double tmp_y = (double)(a1*c2-c1*a2)/(double)(b1*a2-a1*b2);
		CrossP.y = (long) tmp_y;

		if(a1 != 0) 
		{
			double tmp_x = (-c1-b1* tmp_y )/a1;
			CrossP.x = (long) tmp_x;
		}
		else if(a2 !=0)
		{
			double tmp_x = (-c2-b2* tmp_y )/a2;
			CrossP.x = (long) tmp_x;
		}
	}

    return CrossP;	
}

//点在线上
bool IsOnline(CPoint pt,Line line){
	if(Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt.x,pt.y)==0)
		return 1;
	else 
		return 0;
}

//两条线段是否相交
bool Intersect(CPoint pt1,CPoint pt2,Line line){
	int a1=Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt1.x,pt1.y);
	int a2=Multinomial(line.startpoint.x,line.startpoint.y,line.endpoint.x,line.endpoint.y,pt2.x,pt2.y);
	if((a1<0&&a2>0)||(a1>0&&a2<0))
		return 1;
	else
		return 0;
}

//判断点是否在多边形内
bool InPolygon(Boundary & boundary,CPoint point){
	int n=boundary.vertexs.size();
	int CrossCount=0;
	//int Count=0;
	//延某一方向做一条直线
	Line line;
	line.startpoint=point;
	line.endpoint.x=point.x;
	line.endpoint.y=INTIALIZE;
	//得到多边形的边
	for(int i=0;i<n-1;i++){
		Line side;
		side.startpoint=boundary.vertexs[i];
		side.endpoint=boundary.vertexs[(i+1)%n];
		//判断点是否在边上
		if(IsOnline(point,side)) return 1;
		//若该边与Y轴平行 不考虑
		else if(abs(side.startpoint.x-side.endpoint.x)<ESP) continue;
		//讨论过顶点的情况
		else if(IsOnline(side.startpoint,line)){
			if(Intersect(boundary.vertexs[(i+1)%n],boundary.vertexs[(i-1)%n],line)&&side.startpoint.y<point.y) CrossCount++;
			if(i==(n-2)) break;
			//else return false;
		}
		else if(IsOnline(side.endpoint,line)){
			if(Intersect(boundary.vertexs[i],boundary.vertexs[(i+2)%(n-1)],line)&&side.endpoint.y<point.y){
				CrossCount++;
				i++;
			}  
			//else return false;
		}
		else if(Intersect(side.startpoint,side.endpoint,line)){
			if(Intersect(line.startpoint,line.endpoint,side)) CrossCount++;
		} 
	}
	if ( CrossCount==2||CrossCount==0) {return 0;}
	else { return 1;}
}

Line result(Boundary & boundary,Line line){
	Line line_result;
	line_result.startpoint.x=line_result.startpoint.y=INTIALIZE;
	line_result.endpoint.x=line_result.endpoint.y=INTIALIZE;

	bool bspt=InPolygon(boundary,line.startpoint);
	bool bept=InPolygon(boundary,line.endpoint);

 	if(bspt&&bept)
		return line; //若两点都在多边形内则绘制该直线
	else if(bspt&&(!bept)){
		line_result.startpoint=line.startpoint;
		//求交点
		int n=boundary.vertexs.size();
		int i=0;
		while(i<n-1){
			Line side;
			side.startpoint=boundary.vertexs[i];
			side.endpoint=boundary.vertexs[(i+1)%n];
			//过点(x1,y1)(x2,y2)一般式直线方程 (y2-y1)x+(x1-x2)y+x2y1-x1y2=0
			if(Intersect(side.startpoint,side.endpoint,line)){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&p.x>=min(line.startpoint.x,line.endpoint.x)
					&&p.y<=max(line.startpoint.y,line.endpoint.y)&&p.y>=min(line.startpoint.y,line.endpoint.y)){
							line_result.endpoint=CrossPoint(line,side);
							break;
				}
				else i++;
			}
			else i++;
		}
		return line_result;
	}
	else if((!bspt)&&bept){
		line_result.startpoint=line.endpoint;
		//求交点
		int n=boundary.vertexs.size();
		int i=0;
		while(i<n-1){
			Line side;
			side.startpoint=boundary.vertexs[i];
			side.endpoint=boundary.vertexs[(i+1)%n];
			if(Intersect(side.startpoint,side.endpoint,line)){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&p.x>=min(line.startpoint.x,line.endpoint.x)
					&&p.y<=max(line.startpoint.y,line.endpoint.y)&&p.y>=min(line.startpoint.y,line.endpoint.y)){
							line_result.endpoint=CrossPoint(line,side);
							break;
				}
				else i++;
			}
			else i++;
		}
		return line_result;
	}
	else if((!bspt)&&(!bept)){
		int n=boundary.vertexs.size();
		int i=0,nCrossPoint=0;
		while(i<(n-1)&&nCrossPoint!=2){
			Line side;
			side.startpoint=boundary.vertexs[i];
			side.endpoint=boundary.vertexs[(i+1)%n];
			bool isIntersect=Intersect(side.startpoint,side.endpoint,line);
			if(isIntersect&&line_result.startpoint.x==INTIALIZE){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&p.x>=min(line.startpoint.x,line.endpoint.x)
					&&p.y<=max(line.startpoint.y,line.endpoint.y)&&p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.startpoint=p;
				        nCrossPoint++;
				        i++;
				}
				else i++;
			}
			else if(isIntersect&&line_result.endpoint.x==INTIALIZE){
				CPoint p=CrossPoint(line,side);
				if(p.x<=max(line.startpoint.x,line.endpoint.x)&&p.x>=min(line.startpoint.x,line.endpoint.x)
					&&p.y<=max(line.startpoint.y,line.endpoint.y)&&p.y>=min(line.startpoint.y,line.endpoint.y)){
						line_result.endpoint=p;
				        nCrossPoint++;
				        i++;
				}
				else i++;
				/*line_result.endpoint=CrossPoint(line,side);
				nCrossPoint++;
				i++;*/
			}
			else i++;
		}
		return line_result;
	}
}


void CDemo_ClipView_VCDlg::ClearTestLines()
{
	hasOutCanvasData = FALSE;
	complexArray.RemoveAll();
	//boundary.vertexs.clear();
	circles.clear();
	lines.clear();
	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));
}

void CDemo_ClipView_VCDlg::dealConvex()
{

	//TODO 在此处完成裁剪算法和裁剪后显示程序
	vector<Line> result_lines;
	
	//int nlines=lines.size();
	
	for(int i=0;i<lines.size();i++){
		Line real_line=lines[i];
		real_line.startpoint.y=600-lines[i].startpoint.y;
		real_line.endpoint.y=600-lines[i].endpoint.y;
		if(InBox(boundary,real_line)){
			Line line=result(boundary,lines[i]);
			result_lines.push_back(line);
		}
	/*	else{
			Line line;
			line.startpoint.x=line.endpoint.x=INTIALIZE;
			line.startpoint.y=line.endpoint.y=INTIALIZE;
			result_lines.push_back(line);
		}*/
	}

	//EndTimeAndMemoryMonitor();

	ClearTestLines();

	//COLORREF clrBoundary = RGB(255,0,0);
	//DrawBoundary(boundary,clrBoundary);

	COLORREF clrLine = RGB(0,255,0);
	//int nResultLines=result_lines.size();
	for (int i=0;i<result_lines.size();i++)
	{
		if(result_lines[i].startpoint.x!=INTIALIZE && result_lines[i].startpoint.y!=INTIALIZE && result_lines[i].endpoint.x!=INTIALIZE && result_lines[i].endpoint.y!=INTIALIZE)
			DrawLine(result_lines[i], clrLine);
	}

}

// GS's code end
/////////////////////////////////














	
//////////////////////////////////
// DQC's codes begin

//判断每个点是凹点还是凸点
void CDemo_ClipView_VCDlg::JudgeConvexPoint()
{
	int dotnum=boundary.vertexs.size()-1;
	vector<int> z;
	int max=-1,maxnum=-1;
	for (int i=0;i<dotnum;i++)
	{
		CPoint* p1;
		if (i==0)
			p1=&boundary.vertexs[dotnum-1];
		else
			p1=&boundary.vertexs[i-1];
		CPoint* p2=&boundary.vertexs[i];
		CPoint* p3=&boundary.vertexs[i+1];
		int ux=p2->x-p1->x;
		int uy=p2->y-p1->y;
		int vx=p3->x-p2->x;
		int vy=p3->y-p2->y;
		z.push_back(ux*vy-uy*vx);
		if (p2->x>max)
		{
			max=p2->x;
			maxnum=i;
		}
	}

	for (int i=0;i<dotnum;i++)
		if ((z[maxnum]>0 && z[i]>0)||(z[maxnum]<0 && z[i]<0))
			convexPoint.push_back(true);
		else
			convexPoint.push_back(false);
	convexPoint.push_back(convexPoint[0]);
}

int CrossMulti(CPoint a1,CPoint a2,CPoint b1,CPoint b2)
{
	int ux=a2.x-a1.x;
	int uy=a2.y-a1.y;
	int vx=b2.x-b1.x;
	int vy=b2.y-b1.y;
	return ux*vy-uy*vx;
}
bool Compare(IntersectPoint a,IntersectPoint b)
{
	if (a.t<b.t)
		return true;
	else
		return false;
}

void CDemo_ClipView_VCDlg::dealConcave()
{
	//TODO 在此处完成裁剪算法和裁剪后显示程序
	COLORREF clrLine = RGB(0,255,0);
	JudgeConvexPoint();

	//先算出所有多边形的边的矩形框
	int edgenum=boundary.vertexs.size()-1;
	for (int i=0;i<edgenum;i++)
	{
		RECT r;
		r.bottom=min(boundary.vertexs[i].y,boundary.vertexs[i+1].y);
		r.top=max(boundary.vertexs[i].y,boundary.vertexs[i+1].y);
		r.left=min(boundary.vertexs[i].x,boundary.vertexs[i+1].x);
		r.right=max(boundary.vertexs[i].x,boundary.vertexs[i+1].x);
		edgeRect.push_back(r);
	}



	int linenum=lines.size();
	RECT r;
	for (int i=0;i<linenum;i++)//枚举每条线段
	{
		r.bottom=min(lines[i].startpoint.y,lines[i].endpoint.y);
		r.top=max(lines[i].startpoint.y,lines[i].endpoint.y);
		r.left=min(lines[i].startpoint.x,lines[i].endpoint.x);
		r.right=max(lines[i].startpoint.x,lines[i].endpoint.x);

		//将线段起点当作入点放在首位
		IntersectPoint ip1;
		ip1.isIntoPoly=true;
		ip1.t=0;
		ip1.point=lines[i].startpoint;
		intersectPoint.clear();
		intersectPoint.push_back(ip1);

		BOOL haveIntersect=false;
		for (int j=0;j<edgenum;j++)//枚举每条多边形的边
		{
			//快速排除法
			if (r.left>edgeRect[j].right || r.right<edgeRect[j].left ||
				r.top<edgeRect[j].bottom || r.bottom>edgeRect[j].top)
				continue;

			//跨立试验
			CPoint p1=boundary.vertexs[j];
			CPoint p2=boundary.vertexs[j+1];//p1p2为多边形的边
			CPoint q1=lines[i].startpoint;
			CPoint q2=lines[i].endpoint;//q1q2为线段


			long long a= ((long long)CrossMulti(p1,q1,p1,p2)) * (CrossMulti(p1,p2,p1,q2));
			long long b= ((long long)CrossMulti(q1,p1,q1,q2)) * (CrossMulti(q1,q2,q1,p2));
			if (!(a>=0 && b>=0))
				continue;

			//求内法向量
			CPoint p0;
			if (j==0)
				p0=boundary.vertexs[edgenum-1];
			else
				p0=boundary.vertexs[j-1];
			double n1,n2;
			if (p2.y-p1.y==0)
			{
				n2=1;
				n1=0;
			}
			else
			{
				n1=1;
				n2=-(double)(p2.x-p1.x)/(p2.y-p1.y);
			}
			if ((p1.x-p0.x)*n1+(p1.y-p0.y)*n2>0)//法向量反向
			{
				n1=-n1;
				n2=-n2;
			}
			if (!convexPoint[j])//如果是凹点法向量反向
			{
				n1=-n1;
				n2=-n2;
			}


			//cyrus-beck算法 算出参数t
			double t1=n1*(q2.x-q1.x)+n2*(q2.y-q1.y);
			double t2=n1*(q1.x-p1.x)+n2*(q1.y-p1.y);
			double t=-t2/t1;
			CPoint pt;
			pt.x=(LONG)((q2.x-q1.x)*t+q1.x);
			pt.y=(LONG)((q2.y-q1.y)*t+q1.y);
			if (t>=0 && t<=1)
			{
				IntersectPoint ip;
				if (t1>0)
					ip.isIntoPoly=true;
				else
					ip.isIntoPoly=false;
				ip.t=t;
				ip.point=pt;
				ip.convexPointNum=j;
				intersectPoint.push_back(ip);
				
				haveIntersect=true;
			}
			else
			{
				//error
				int a=0;
			}
		}

		
		//没有交点，则跳过
		if (!haveIntersect)
			continue;

		//将线段终点当作出点放在末位
		IntersectPoint ip2;
		ip2.isIntoPoly=false;
		ip2.t=1;
		ip2.point=lines[i].endpoint;
		intersectPoint.push_back(ip2);

		std::sort(intersectPoint.begin()+1,intersectPoint.end()-1,Compare);//按t从小到大排序

		//如果有两点t值相同（即是同一点），根据点的凹凸性重新安排入、出
		vector<IntersectPoint>::iterator iter = intersectPoint.begin();
		iter++;
		for (;iter!= intersectPoint.end()&&(iter+1)!= intersectPoint.end()&&(iter+2)!= intersectPoint.end();iter++ )
			if ( ((*iter).isIntoPoly ^ (*(iter+1)).isIntoPoly )
				&&abs((*iter).t-(*(iter+1)).t)<1e-9)
			{
				if (convexPoint[(*iter).convexPointNum+1]&& !(*iter).isIntoPoly)
				{
					(*iter).isIntoPoly=true;
					(*(iter+1)).isIntoPoly=false;
				}
				else if (!convexPoint[(*iter).convexPointNum+1] && (*iter).isIntoPoly)
				{
					(*iter).isIntoPoly=false;
					(*(iter+1)).isIntoPoly=true;
				}

			}


		//以下为暂时的绘图代码
		int size=(int)intersectPoint.size()-1;
		for (int i=0;i<size;i++)
		{
			if (intersectPoint[i].isIntoPoly && !intersectPoint[i+1].isIntoPoly)
			{
				Line l;
				l.startpoint=intersectPoint[i].point;
				l.endpoint=intersectPoint[i+1].point;
				DrawLine(l,clrLine);
				i++;
			}
		}
		//以上为暂时的绘图代码
	}

}

// DQC's code end
/////////////////////////////////













//////////////////////////////////
// XH's codes begin

void  CDemo_ClipView_VCDlg::forCircleRun()
{
	//ClearPartialTestCaseData();
	//COLORREF clrBoundary = RGB(255,0,0);
	//DrawBoundary(boundary,clrBoundary);
	for(unsigned int i = 0;i<circles.size();i++){
	    vector<r_lineNum> point_Array;
		getInterpointArray(point_Array,i);
		
		/*if (point_Array.size()>=2)
		{
			for (int j = 0; j < point_Array.size()-1; j++)
		    {
			    if (point_Array[j].point.x ==point_Array[j+1].point.x&&point_Array[j].point.y ==point_Array[j+1].point.y)
				{
					point_Array.erase(point_Array.begin() + j);
					j--;
				}
		    }
		}*/
		if(point_Array.size()==0||point_Array.size()==1)
		{
			bool inBoundary = isPointInBoundary(circles[i].center);
			if (inBoundary==true)
			{
				long _x = boundary.vertexs[0].x-circles[i].center.x;
				long _y = boundary.vertexs[0].y-circles[i].center.y;
				bool t = (_x*_x+_y*_y)>(circles[i].radius*circles[i].radius);
				if (t==true)
				{
					//画整个圆
					COLORREF clrCircle = RGB(0,0,255);
					DrawCircle(circles[i],clrCircle);

				}
				else
				{
					//什么都不画
					continue;
				}
			}
		}
		//没有交点的情况讨论完毕
		else
		{
			
			point_Array.push_back(point_Array[0]);
			for (int j = 0; j < point_Array.size()-1; j++)
			{
				if (point_Array[j].point.x ==point_Array[j+1].point.x&&point_Array[j].point.y ==point_Array[j+1].point.y)
				{
					point_Array.erase(point_Array.begin() + j);
					j--;
				}
			}
			for (unsigned int j = 0; j < point_Array.size()-1; j++)
			{
				CPoint mid_point = getMiddlePoint(point_Array,i,j);
				bool mid_in_bound = isPointInBoundary(mid_point);
				if(mid_in_bound == true)
				{
					//该弧在多边形框内，画整段弧
					/*double a1 = getAngle(point_Array[j].point.x,circles[i].center.x,point_Array[j].point.y,circles[i].center.y,circles[i].radius);
	                double a2 = getAngle(point_Array[j+1].point.x,circles[i].center.x,point_Array[j+1].point.y,circles[i].center.y,circles[i].radius);*/
					/*double angle1 = -(a1/2*PI)*360;
					double angle2 = -(a2/2*PI)*360;*/
					COLORREF clrCircle = RGB(0,0,255);					
					int start_x = circles[i].center.x-circles[i].radius;
					int start_y = circles[i].center.y-circles[i].radius;
					int end_x =  circles[i].center.x+circles[i].radius;
					int end_y = circles[i].center.y+circles[i].radius;
					CClientDC dc(this);
					CPen penUse;
					penUse.CreatePen(PS_SOLID, 1, clrCircle);
	                CPen* penOld = dc.SelectObject(&penUse);
					CRect rect(start_x,start_y,end_x,end_y);
					dc.Arc(&rect,point_Array[j].point,point_Array[j+1].point);
					dc.SelectObject(penOld);
				}
				else
				{
					if(point_Array[j].num_line==point_Array[j+1].num_line)
					{
						//该弧在多边形外
						//该弧的两交点在一条多边形的边上
						//画两交点的连线
						CClientDC dc(this);
						CPen penUse;
						COLORREF clrLine = RGB(0,0,255);
						penUse.CreatePen(PS_SOLID, 1, clrLine);
						CPen* penOld = dc.SelectObject(&penUse);

						dc.MoveTo(point_Array[j].point);
						dc.LineTo(point_Array[j+1].point);

						dc.SelectObject(penOld);
					}					
				}
			}
		}

	}
}
void CDemo_ClipView_VCDlg::ClearPartialTestCaseData()
{
	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));
}
double CDemo_ClipView_VCDlg::getAngle(long x1,long x2,long y1,long y2,double r)
{
	double cos = (double)(x1-x2)/(double)r;
	double sin = (double)(y1-y2)/(double)r;
	if (sin>0)
	{
		double a = acos(cos);
		return a;
	}
	else
	{
		double a = 2*PI - acos(cos);
		return a;
	}
}
CPoint CDemo_ClipView_VCDlg::getMiddlePoint(vector<r_lineNum>& point_Array,int i,int j)
{
	double a1 = getAngle(point_Array[j].point.x,circles[i].center.x,point_Array[j].point.y,circles[i].center.y,circles[i].radius);
	double a2 = getAngle(point_Array[j+1].point.x,circles[i].center.x,point_Array[j+1].point.y,circles[i].center.y,circles[i].radius);
	double mid_a = 0;

	if(a1<PI&&a2<PI)
	{
		if(a1>a2)
		{
			mid_a = (a1+a2)/2;
		}
		else
		{
			mid_a = (a1+a2)/2+PI;
		}
		
	}
	else if (a1<PI&&a2>PI)
	{
		if(a1+a2>2*PI)
		{
			mid_a = (a1+a2)/2-PI;
		}
		else
		{
			mid_a = (a1+a2)/2+PI;
		}
				
	}
	else if (a2<PI&&a1>PI)
	{
		
		mid_a = (a1+a2)/2;
	}
	else if (a2>PI&&a1>PI)
	{
		if(a1>a2)
		{
			mid_a = (a1+a2)/2;
		}
		else
		{
			mid_a = (a1+a2)/2-PI;
		}
		
	}
	double x = circles[i].center.x+circles[i].radius*cos(mid_a);
	double y = circles[i].center.y+circles[i].radius*sin(mid_a);
	CPoint point ;
	point.x = x;
	point.y = y;
	return point;
}
bool CDemo_ClipView_VCDlg::isPointInBoundary(CPoint& point)
{
	int interNum = 0;
	long x1 = point.x;
	long y1 = point.y;
	long x2 = x1;
	long y2 = 0;
	for (unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		if (boundary.vertexs[i].x==x1&&boundary.vertexs[i].y<=y1)
		{
			if (i==(boundary.vertexs.size()-1))
			{
				long between =(boundary.vertexs[i-1].x-x1)*(boundary.vertexs[1].x-x1);
				if (between<0)
				{
					interNum++;
				}

			}
			else
			{
				long between =(boundary.vertexs[i-1].x-x1)*(boundary.vertexs[i+1].x-x1);
				if (between<0)
				{
					interNum++;
				}
			}
		}
	}
	for(unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		long between =(boundary.vertexs[i].x-x1)*(boundary.vertexs[i-1].x-x1);
		if(between<0)
		{
			long x3 = boundary.vertexs[i].x;
		    long x4 = boundary.vertexs[i-1].x;
			long y3 = boundary.vertexs[i].y;
			long y4 = boundary.vertexs[i-1].y;
			long a = y4-y3;
			long b = x3-x4;
			long c = x4*y3-x3*y4;
			long long isInter = (long long)(a*x1+b*y1+c)*(long long)(a*x2+b*y2+c);
			if(isInter<0)
			{
				interNum++;
			}

		}
	}
	if(interNum%2!=0)
	{
		return true;
	}
	else
	{
		return false;
	}

}
struct r_lineNum CDemo_ClipView_VCDlg::getInterpoint(double t,int x1,int x2,int y1,int y2,int i)
{
	long x = x1+(x2-x1)*t;
	long y = y1+(y2-y1)*t;
    struct r_lineNum pit;
	pit.point.x = x;
	pit.point.y = y;
	pit.num_line = i;
	return pit;
}

void CDemo_ClipView_VCDlg::getInterpointArray(vector<r_lineNum>& point_Array,int circle_num)
{
	for(unsigned int i =(boundary.vertexs.size()-1);i>0;i--)
	{
		long x1 = boundary.vertexs[i].x;
		long x2 = boundary.vertexs[i-1].x;
		long y1 = boundary.vertexs[i].y;
		long y2 = boundary.vertexs[i-1].y;
		long x0 = circles[circle_num].center.x;
		long y0 = circles[circle_num].center.y;
		long a = y2-y1;
		long b = x1-x2;
		long c = x2*y1-x1*y2;
		double _abs = abs((double)(a*x0+b*y0+c));
		double _sqrt = sqrt((double)(a*a+b*b));
		double d = _abs/_sqrt;
		if (d>circles[circle_num].radius)
		{
			continue;
		}
		long long A = x1*x1+x2*x2+y1*y1+y2*y2-2*x1*x2-2*y1*y2;
		long long B = 2*(x0*x1+x1*x2+y0*y1+y1*y2-x0*x2-y0*y2-x1*x1-y1*y1);
		long long C = x0*x0+x1*x1+y0*y0+y1*y1-2*x0*x1-2*y0*y1-circles[circle_num].radius*circles[circle_num].radius;
		long long key = B*B-4*A*C;
		if(key == 0)
		{
			double t = ((double)(-B)/(double)(2*A));
			if (t>=0&&t<=1)
			{
				struct r_lineNum pit = getInterpoint(t,x1,x2,y1,y2,i);
				point_Array.push_back(pit);
			}
			else
			{
				continue;
			}
		}
		else if(key > 0)
		{
			double t1 = (double)((-B+sqrt(key))/(2*A));
			double t2 = (double)((-B-sqrt(key))/(2*A));
			if (t1>t2)
			{
				double tmp = t2;
				t2 = t1;
				t1 =tmp;
			}
			if (t1>=0&&t1<=1)
			{				
				struct r_lineNum pit = getInterpoint(t1,x1,x2,y1,y2,i);
 				point_Array.push_back(pit);
				if (t2>=0&&t2<=1)
				{
					struct r_lineNum pit = getInterpoint(t2,x1,x2,y1,y2,i);
				    point_Array.push_back(pit);
				}

			}
			else
			{
				if (t2>=0&&t2<=1)
				{
					struct r_lineNum pit = getInterpoint(t2,x1,x2,y1,y2,i);
				    point_Array.push_back(pit);
				}
				else
				{
					continue;
				}
			}
		}
	}
}




// XH's code end
/////////////////////////////////














CDemo_ClipView_VCDlg::CDemo_ClipView_VCDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDemo_ClipView_VCDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDemo_ClipView_VCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_CLIP, m_btn_clip);
	DDX_Control(pDX, IDC_STATIC_DRAWING, m_stc_drawing);
	DDX_Control(pDX, IDC_STATIC_INFO_1, m_stc_info1);
	DDX_Control(pDX, IDC_STATIC_INFO_2, m_stc_info2);
}

BEGIN_MESSAGE_MAP(CDemo_ClipView_VCDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CLIP, &CDemo_ClipView_VCDlg::OnBnClickedBtnClip)
	ON_WM_NCLBUTTONDOWN()
END_MESSAGE_MAP()


BOOL CDemo_ClipView_VCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	CRect clientRect;
	GetClientRect(&clientRect);
	CRect windowRect;
	GetWindowRect(&windowRect);
	int width = CANVAS_WIDTH + windowRect.Width() - clientRect.Width();
	int height = CANVAS_HEIGHT + INFO_HEIGHT + windowRect.Height() - clientRect.Height();
	SetWindowPos(NULL, 0, 0, width, height, SWP_NOMOVE|SWP_NOZORDER);

	m_btn_clip.SetWindowPos(NULL, 50, CANVAS_HEIGHT + 10, 80, 30, SWP_NOZORDER);
	m_stc_drawing.SetWindowPos(NULL, 140, CANVAS_HEIGHT + 15, 200, 20, SWP_NOZORDER);
	m_stc_info1.SetWindowPos(NULL, 350, CANVAS_HEIGHT + 5, 450, 20, SWP_NOZORDER);
	m_stc_info2.SetWindowPos(NULL, 350, CANVAS_HEIGHT + 25, 450, 20, SWP_NOZORDER);

	hasOutCanvasData = FALSE;
	GetModuleFileName(NULL,curPath.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
	curPath.ReleaseBuffer();
	curPath = curPath.Left(curPath.ReverseFind('\\') + 1);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CDemo_ClipView_VCDlg::OnPaint()
{

	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CDemo_ClipView_VCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CDemo_ClipView_VCDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT uMsg=LOWORD(wParam);
	switch(uMsg)
	{
	case ID_TESTCASE1:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "1");
		}
		break;
	case ID_TESTCASE2:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "2");
		}
		break;
	case ID_TESTCASE3:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "3");
		}
		break;
	case ID_TESTCASE4:
		{
			CString xmlPath = curPath + TESTDATA_XML1;
			DrawTestCase(xmlPath, "4");
		}
		break;
	case ID_TESTCASE5:
		{
			CString xmlPath = curPath + TESTDATA_XML2;
			DrawTestCase(xmlPath, "5");
		}
		break;
	default:
		break;
	}
	return CDialogEx::OnCommand(wParam, lParam);
}

void CDemo_ClipView_VCDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	if (HTCAPTION == nHitTest) {
		return;
	}
	CDialogEx::OnNcLButtonDown(nHitTest, point);
}



BOOL CDemo_ClipView_VCDlg::XmlNodeToPoint(pugi::xml_node node, CPoint& piont)
{
	pugi::xml_node pointNode = node.first_child();
	CString strValue(pointNode.value());
	int curPos = 0;
	int count = 0;
	while(TRUE)
	{
		CString resToken = strValue.Tokenize(",", curPos);
		if (resToken.IsEmpty())
			break;
		int coord = _ttoi(resToken);
		count++;
		if (count == 1)
		{
			piont.x = coord;
		}
		else if (count == 2)
		{
			piont.y = CANVAS_HEIGHT - coord; //调整CAD与窗口显示的坐标系一致
		}
	}
	if (count != 2)
	{
		return FALSE;
	}
	return TRUE;
}


int  CDemo_ClipView_VCDlg::SplitCStringToArray(CString str,CStringArray& strArray, CString split)
{
	int curPos = 0;
	while(TRUE)
	{
		CString resToken = str.Tokenize(split, curPos);
		if (resToken.IsEmpty())
			break;
		strArray.Add(resToken);
	}
	return strArray.GetSize();
}
BOOL CDemo_ClipView_VCDlg::IsPointOutCanvas(CPoint point)
{
	if (point.x < 0 || point.y < 0 || point.x > CANVAS_WIDTH || point.y > CANVAS_HEIGHT)
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDemo_ClipView_VCDlg::IsLineOutCanvas(Line line)
{
	if (IsPointOutCanvas(line.startpoint) || IsPointOutCanvas(line.endpoint))
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDemo_ClipView_VCDlg::IsCircleOutCanvas(Circle circle)
{
	if (IsPointOutCanvas(circle.center))
	{
		return TRUE;
	}
	if ((circle.center.x - circle.radius) < 0 || (circle.center.x + circle.radius) > CANVAS_WIDTH
		|| (circle.center.y - circle.radius) < 0 || (circle.center.y + circle.radius) > CANVAS_HEIGHT)
	{
		return TRUE;
	}
	return FALSE;
}
BOOL CDemo_ClipView_VCDlg::IsBoundaryOutCanvas(Boundary boundary)
{
	vector<CPoint>::iterator iter = boundary.vertexs.begin();
	for (;iter != boundary.vertexs.end(); iter++)
	{
		if (IsPointOutCanvas(*iter))
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL CDemo_ClipView_VCDlg::LoadTestCaseData(CString xmlPath, CString caseID)
{
	pugi::xml_document doc;
	if (!doc.load_file(xmlPath)) return FALSE;
	pugi::xml_node root = doc.child("TestRoot");
	pugi::xml_node testCase = root.find_child_by_attribute("ID",caseID);
	if (!testCase) return FALSE;
	pugi::xml_attribute complex = testCase.attribute("complex");
	if (!complex) return FALSE;
	CString strComplex = complex.value();
	if (SplitCStringToArray(strComplex, complexArray, ",") != 6)
	{
		complexArray.RemoveAll();
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
		complexArray.InsertAt(0, " ");
	}
	for (pugi::xml_node entity = testCase.first_child(); entity; entity = entity.next_sibling())
	{
		pugi::xml_attribute type = entity.attribute("Type");
		CString typeValue(type.value());
		if (typeValue.CompareNoCase("Line") == 0)
		{
			Line line;
			pugi::xml_node startNode = entity.child("StartPoint");
			if(!XmlNodeToPoint(startNode, line.startpoint)) return FALSE;
			pugi::xml_node endNode = entity.child("EndPoint");
			if(!XmlNodeToPoint(endNode, line.endpoint)) return FALSE;
			if (IsLineOutCanvas(line))
			{
				hasOutCanvasData = TRUE;
				continue;
			}
			lines.push_back(line);
		} 
		else if(typeValue.CompareNoCase("Circle") == 0)
		{
			Circle circle;
			pugi::xml_node centerNode = entity.child("CenterPoint");
			if(!XmlNodeToPoint(centerNode, circle.center)) return FALSE;
			pugi::xml_node radiusNode = entity.child("Radius");
			pugi::xml_node pointNode = radiusNode.first_child();
			circle.radius = _ttoi(pointNode.value());
			if (IsCircleOutCanvas(circle))
			{
				hasOutCanvasData = TRUE;
				continue;
			}
			circles.push_back(circle);
		}
		else if (typeValue.CompareNoCase("Convex Polygon") == 0)
		{
			boundary.isConvex = true;
			for (pugi::xml_node vertex = entity.first_child(); vertex; vertex = vertex.next_sibling())
			{
				CPoint point;
				if(!XmlNodeToPoint(vertex, point)) return FALSE;
				boundary.vertexs.push_back(point);
			}
			if (IsBoundaryOutCanvas(boundary))
			{
				hasOutCanvasData = TRUE;
				boundary.vertexs.clear();
			}
		}
		else if (typeValue.CompareNoCase("Concave Polygon") == 0)
		{
			boundary.isConvex = false;
			for (pugi::xml_node vertex = entity.first_child(); vertex; vertex  = vertex.next_sibling())
			{
				CPoint point;
				if(!XmlNodeToPoint(vertex, point)) return FALSE;
				boundary.vertexs.push_back(point);
			}
			if (IsBoundaryOutCanvas(boundary))
			{
				hasOutCanvasData = TRUE;
				boundary.vertexs.clear();
			}
		}
	}
	return TRUE;
}
void CDemo_ClipView_VCDlg::ClearTestCaseData()
{
	hasOutCanvasData = FALSE;
	complexArray.RemoveAll();
	boundary.vertexs.clear();
	circles.clear();
	lines.clear();

	//added
	convexPoint.clear();
	edgeRect.clear();
	intersectPoint.clear();

	CClientDC dc(this);
	dc.FillSolidRect(0,0,CANVAS_WIDTH,CANVAS_HEIGHT, RGB(0,0,0));
}
void CDemo_ClipView_VCDlg::BeginTimeAndMemoryMonitor()
{
	m_btn_clip.EnableWindow(FALSE);
	m_stc_drawing.SetWindowText("裁剪中...");
	m_stc_info1.ShowWindow(SW_HIDE);
	m_stc_info2.ShowWindow(SW_HIDE);
	HANDLE handle=GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS pmc;    
	GetProcessMemoryInfo(handle,&pmc,sizeof(pmc));
	startMemory = pmc.PagefileUsage;
	startTime = clock();
}
void CDemo_ClipView_VCDlg::EndTimeAndMemoryMonitor()
{
	endTime = clock();
	HANDLE handle=GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS pmc;    
	GetProcessMemoryInfo(handle,&pmc,sizeof(pmc));
	endMemory = pmc.PagefileUsage;
	double useTimeS = (endTime - startTime)/1000;
	double useMemoryM = (endMemory - startMemory)/1024/1024;
	m_stc_drawing.SetWindowText("裁剪完毕！");
	m_stc_info1.ShowWindow(SW_SHOW);
	CString strTime;
	strTime.Format("耗时:  %lfs", useTimeS);
	m_stc_info1.SetWindowText(strTime);
	m_stc_info2.ShowWindow(SW_SHOW);
	CString strMemory;
	strMemory.Format("占用内存:  %lfM", useMemoryM);
	m_stc_info2.SetWindowText(strMemory);
}
void CDemo_ClipView_VCDlg::DrawTestCase(CString xmlPath, CString caseID)
{
	m_stc_drawing.ShowWindow(SW_SHOW);
	m_stc_drawing.SetWindowText("图形绘制中...");
	m_stc_info1.ShowWindow(SW_HIDE);
	m_stc_info2.ShowWindow(SW_HIDE);
	ClearTestCaseData();
	if (!LoadTestCaseData(xmlPath, caseID))
	{
		MessageBox("读取数据失败!","Demo_CliView_VC",MB_OK);
		m_stc_drawing.SetWindowText("");
		return;
	}

	COLORREF clrLine = RGB(0,255,0);
	vector<Line>::iterator iterLine = lines.begin();
	for (;iterLine != lines.end(); iterLine++)
	{
		DrawLine(*iterLine, clrLine);
	}

	COLORREF clrCircle = RGB(0,0,255);
	vector<Circle>::iterator iterCircle = circles.begin();
	for (;iterCircle != circles.end(); iterCircle++)
	{
		DrawCircle(*iterCircle, clrCircle);
	}

	COLORREF clrBoundary = RGB(255,0,0);
	DrawBoundary(boundary, clrBoundary);

	if (hasOutCanvasData)
	{
		m_stc_drawing.SetWindowText("存在超出边界数据！！！");
	}
	else
	{
		m_btn_clip.EnableWindow(TRUE);
		m_stc_drawing.SetWindowText("图形绘制完成：");
		m_stc_info1.ShowWindow(SW_SHOW);
		CString strInfo1;
		strInfo1.Format("共有%s个图形和%s个边界，其中%s个图形与边界相交。",complexArray.GetAt(0),complexArray.GetAt(1),complexArray.GetAt(2));
		m_stc_info1.SetWindowText(strInfo1);
		m_stc_info2.ShowWindow(SW_SHOW);
		CString strInfo2;
		strInfo2.Format("共有%s个图形与边界无交点，其中%s个位于边界内，%s个位于边界外。",complexArray.GetAt(3),complexArray.GetAt(4),complexArray.GetAt(5));
		m_stc_info2.SetWindowText(strInfo2);
	}
}


void CDemo_ClipView_VCDlg::DrawLine(Line line, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);

	dc.MoveTo(line.startpoint);
	dc.LineTo(line.endpoint);

	dc.SelectObject(penOld);
}
void CDemo_ClipView_VCDlg::DrawCircle(Circle circle, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);

	dc.Arc(circle.center.x - circle.radius, circle.center.y - circle.radius, circle.center.x + circle.radius, circle.center.y + circle.radius, 0, 0, 0, 0);

	dc.SelectObject(penOld);
}
void CDemo_ClipView_VCDlg::DrawBoundary(Boundary boundary, COLORREF clr)
{
	CClientDC dc(this);
	CPen penUse;
	penUse.CreatePen(PS_SOLID, 1, clr);
	CPen* penOld = dc.SelectObject(&penUse);


	vector<CPoint>::iterator iter = boundary.vertexs.begin();
	if (iter == boundary.vertexs.end()) return;
	CPoint startPoint = *iter;
	dc.MoveTo(startPoint);
	for (;iter != boundary.vertexs.end(); iter++)
	{
		dc.LineTo(*iter);
	}
	dc.LineTo(startPoint);

	dc.SelectObject(penOld);
}