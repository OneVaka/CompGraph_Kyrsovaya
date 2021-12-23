#include "Render.h"

#include <sstream>
#include <iostream>

#include <windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>

#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "GUItextRectangle.h"

void Paint();
void draw_Prizm();

GLdouble* get_Normal(double* point_C,double* point_A,double* point_B,bool reverse = false);

bool textureMode = false;
bool lightMode = true;

double height = 2;
double size = 2;
int steps = 1000;

#pragma region Настройки
//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera()
	{
		//отвечает за поворот камеры мышкой
		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		//функция настройки камеры
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		glDisable(GL_LIGHTING);

		
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();
		
		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}

	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света




//старые координаты мыши
int mouseX = 0, mouseY = 0;

void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}

	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

void mouseWheelEvent(OpenGL *ogl, int delta)
{

	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;

}

void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 189) {
		height--;
		size--;
	}
	if (key == 187) {
		height++;
		size++;
	}

	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}
}

void keyUpEvent(OpenGL *ogl, int key)
{
	
}



GLuint texId;
GLuint texId_2;

void LoadIMG(char* img_name,GLuint* img_ID) {
	//массив трехбайтных элементов  (R G B)
	RGBTRIPLE* texarray;

	//массив символов, (высота*ширина*4      4, потомучто   выше, мы указали использовать по 4 байта на пиксель текстуры - R G B A)
	char* texCharArray;
	int texW, texH;
	OpenGL::LoadBMP(img_name, &texW, &texH, &texarray);
	OpenGL::RGBtoChar(texarray, texW, texH, &texCharArray);

	//генерируем ИД для текстуры
	glGenTextures(1, img_ID);
	//биндим айдишник, все что будет происходить с текстурой, будте происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, *img_ID);

	//загружаем текстуру в видеопямять, в оперативке нам больше  она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);

	//отчистка памяти
	free(texCharArray);
	free(texarray);


	//наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
}

//выполняется перед первым рендером
void initRender(OpenGL *ogl)
{
	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);
	
	LoadIMG("TS.bmp", &texId);
	LoadIMG("texture.bmp", &texId_2);

	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH); 


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	camera.fi1 = -1.3;
	camera.fi2 = 0.8;
}
#pragma endregion




void Render(OpenGL *ogl)
{


	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);


	//альфаналожение
	if(!textureMode){
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;


	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec); \
		//размер блика
		glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH);
	//===================================
	//Прогать тут  


	//Начало рисования квадратика станкина
	//double A[2] = { -4, -4 };
	//double B[2] = { 4, -4 };
	//double C[2] = { 4, 4 };
	//double D[2] = { -4, 4 };
	//
	//glBindTexture(GL_TEXTURE_2D, texId);
	//
	//glColor3d(0.6, 0.6, 0.6);
	//glBegin(GL_QUADS);
	//
	//glNormal3d(0, 0, 1);
	//glTexCoord2d(0, 0);
	//glVertex2dv(A);
	//glTexCoord2d(1, 0);
	//glVertex2dv(B);
	//glTexCoord2d(1, 1);
	//glVertex2dv(C);
	//glTexCoord2d(0, 1);
	//glVertex2dv(D);
	//
	//glEnd();
	//конец рисования квадратика станкина

	//Paint();

	draw_Prizm();

   //Сообщение вверху экрана

	
	glMatrixMode(GL_PROJECTION);	//Делаем активной матрицу проекций. 
	                                //(всек матричные операции, будут ее видоизменять.)
	glPushMatrix();   //сохраняем текущую матрицу проецирования (которая описывает перспективную проекцию) в стек 				    
	glLoadIdentity();	  //Загружаем единичную матрицу
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);	 //врубаем режим ортогональной проекции

	glMatrixMode(GL_MODELVIEW);		//переключаемся на модел-вью матрицу
	glPushMatrix();			  //сохраняем текущую матрицу в стек (положение камеры, фактически)
	glLoadIdentity();		  //сбрасываем ее в дефолт

	glDisable(GL_LIGHTING);



	GuiTextRectangle rec;		   //классик моего авторства для удобной работы с рендером текста.
	rec.setSize(300, 150);
	rec.setPosition(10, ogl->getHeight() - 150 - 10);


	std::stringstream ss;
	ss << "T - вкл/выкл текстур" << std::endl;
	ss << "L - вкл/выкл освещение" << std::endl;
	ss << "F - Свет из камеры" << std::endl;
	ss << "G - двигать свет по горизонтали" << std::endl;
	ss << "G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << "Коорд. света: (" << light.pos.X() << ", " << light.pos.Y() << ", " << light.pos.Z() << ")" << std::endl;
	ss << "Коорд. камеры: (" << camera.pos.X() << ", " << camera.pos.Y() << ", " << camera.pos.Z() << ")" << std::endl;
	ss << "Параметры камеры: R="  << camera.camDist << ", fi1=" << camera.fi1 << ", fi2=" << camera.fi2 << std::endl;
	
	rec.setText(ss.str().c_str());
	rec.Draw();

	glMatrixMode(GL_PROJECTION);	  //восстанавливаем матрицы проекции и модел-вью обратьно из стека.
	glPopMatrix();


	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}

void Paint()
{
	double A[] = { 5, 2, 9 };
	double B[] = { 2, 5, 9 };
	double C[] = { 6, 6, 1 };

	double point[] = { 0,0,3 };

	{
		glBegin(GL_QUADS);


		glNormal3d(0, 0, -1);
		//glColor3d(0.5, 0.2, 0.9);
		glVertex3d(1, 1, 0);
		glVertex3d(1, -1, 0);
		glVertex3d(-1, -1, 0);
		glVertex3d(-1, 1, 0);

		glNormal3d(0, 1, 0);
		//glColor3d(0.9, 0.5, 0.2);
		glVertex3d(1, 1, 0);
		glVertex3d(-1, 1, 0);
		glVertex3d(-1, 1, 1);
		glVertex3d(1, 1, 1);

		glNormal3d(0, -1, 0);
		//
		glVertex3d(1, -1, 0);
		glVertex3d(-1, -1, 0);
		glVertex3d(-1, -1, 1);
		glVertex3d(1, -1, 1);

		glNormal3d(1, 0, 0);
		//
		glVertex3d(1, -1, 0);
		glVertex3d(1, 1, 0);
		glVertex3d(1, 1, 1);
		glVertex3d(1, -1, 1);

		glNormal3d(-1, 0, 0);
		//
		glVertex3d(-1, -1, 0);
		glVertex3d(-1, 1, 0);
		glVertex3d(-1, 1, 1);
		glVertex3d(-1, -1, 1);

		glEnd();
	}


	{
		glBegin(GL_TRIANGLES);

		//glColor3d(0.5, 0.5, 0.5);

		glNormal3d(4, 0, 3);
		glVertex3dv(point);
		glVertex3d(1, 1, 1);
		glVertex3d(1, -1, 1);

		glNormal3d(0, 4, 3);
		glVertex3dv(point);
		glVertex3d(1, 1, 1);
		glVertex3d(-1, 1, 1);

		glNormal3d(-4, 0, 3);
		glVertex3dv(point);
		glVertex3d(-1, 1, 1);
		glVertex3d(-1, -1, 1);

		glNormal3d(0, -4, 3);
		glVertex3dv(point);
		glVertex3d(-1, -1, 1);
		glVertex3d(1, -1, 1);


		glEnd();
	}




}


void draw_half_Circle(double* point_A, double* point_B,bool stena = false) {

	
	const double angle_Beta = 3.1415926 / steps;


	double point_center[3] = {
		(point_A[0] + point_B[0]) / 2,
		(point_A[1] + point_B[1]) / 2,
		(point_A[2] + point_B[2]) / 2,
	};


	double R = sqrt(pow((point_B[0] - point_center[0]), 2) + pow((point_B[1] - point_center[1]), 2)); //+pow((point_B[2] - point_center[2]), 2));

	double angle_point[2] = { point_B[0] - point_center[0], point_B[1] - point_center[1] };

	double init_angle = acos((angle_point[0] * 0 + angle_point[1] * 1) / (sqrt(pow(angle_point[0], 2) + pow(angle_point[1], 2)) * sqrt(pow(0, 2) + pow(1, 2))));
	//daet radian
	//double test = acos(0.65138);

	glBegin(GL_TRIANGLE_FAN);
		//glNormal3dv(get_Normal(points_arr[0], points_arr[1], points_arr[2], 1));

		glVertex3dv(point_center);
		for (int i = 0; i <= steps; i++) {

			glVertex3d(
				point_center[0] + R * sin((angle_Beta * i) - init_angle),
				point_center[1] + R * cos((angle_Beta * i) - init_angle),
				point_center[2]
			);
			
		}
	glEnd();
	
	if (stena) {
		//glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_2D, texId_2);
		glBegin(GL_QUADS);
		//glColor4d(0.6, 0.2, 0.6, 1);
		glColor4d(0.5, 0.5, 0.5, 1);
		double prev_point[3] = { point_A[0],point_A[1],point_A[2] - height }; //костыль
		double tex_X = 1;
		tex_X /= steps;
		//double tex_XX = tex_X / steps;
		for (int i = 1; i <= steps; i++) {


			double new_X = point_center[0] + R * sin((angle_Beta * i) - init_angle);
			double new_Y = point_center[1] + R * cos((angle_Beta * i) - init_angle);

			double point_1[] = { prev_point[0], prev_point[1], point_center[2] - height };
			double point_2[] = { new_X, new_Y, point_center[2] - height };
			double point_3[] = { prev_point[0], prev_point[1], point_center[2] };
			double point_4[] = { new_X, new_Y, point_center[2] };
			glNormal3dv(get_Normal(point_1, point_2, point_3, 1));



			glTexCoord2d(tex_X * (double(i) - 1), 0);
			glVertex3dv(point_1);
			glTexCoord2d(tex_X * (double(i) - 1), 1);
			glVertex3dv(point_3);
			glTexCoord2d(tex_X * double(i), 1);
			glVertex3dv(point_4);
			glTexCoord2d(tex_X * double(i), 0);
			glVertex3dv(point_2);

			prev_point[0] = new_X; prev_point[1] = new_Y;
			
		}

		glEnd();
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

//void draw_Figure(std::vector<double*> points_arr) {
//	glBegin(GL_TRIANGLE_FAN);
//	glColor4d(0.5, 0.8, 0.5, 1);
//	glNormal3dv(get_Normal(points_arr[0], points_arr[1], points_arr[2], 1));
//	glVertex3dv(points_arr[0]);
//	for (int i = 1; i < points_arr.size(); i++) {
//		glVertex3dv(points_arr[i]);
//	}
//	glVertex3dv(points_arr[1]);
//	glEnd();
//}

void draw_Vipyk(double* point_A, double* point_B) {

	double point_C[] = {-2.5*size,1.5*size,0};

	double A = point_A[0] - point_C[0];
	double B = point_A[1] - point_C[1];
	double C = point_B[0] - point_C[0];
	double D = point_B[1] - point_C[1];

	double E = A * (point_C[0] + point_A[0]) + B * (point_A[1] + point_C[1]);
	double F = C * (point_C[0] + point_B[0]) + D * (point_C[1] + point_B[1]);
	double G = A*D - B*C;


	//центр окружности
	double x = (D * E - B * F) / (2 * G);
	double y = (A * F - C * E) / (2 * G);

	double R = sqrt(pow((point_B[0] - x), 2) + pow((point_B[1] - y), 2));

	const double angle_Beta = 2*3.1415926 / steps;

	
	//std::vector<double*> okr_points;

	//okr_points.push_back(point_A);
	for (int i = 0; i <= steps; i++) {
		double new_X = x + R * sin(angle_Beta * i);
		double new_Y = y + R * cos(angle_Beta * i);
		if ((new_X >= point_B[0]) && (new_Y <= point_A[1])) {
			
			glVertex3d(
				new_X,
				new_Y,
				point_A[2]
			);
			//double point[3];
			//
			//point[0] = new_X;
			//point[1] = new_Y;
			//point[2] = point_A[2];
			//okr_points.push_back(new double[] {new_X,new_Y,point_A[2]});
		}

	}

	//okr_points.push_back(point_B);
	//return okr_points;
}

void draw_Vpyk_Walls(double* point_A, double* point_B, double height) {

	double point_C[] = { -2.5*size,1.5*size,0 };

	double A = point_A[0] - point_C[0];
	double B = point_A[1] - point_C[1];
	double C = point_B[0] - point_C[0];
	double D = point_B[1] - point_C[1];

	double E = A * (point_C[0] + point_A[0]) + B * (point_A[1] + point_C[1]);
	double F = C * (point_C[0] + point_B[0]) + D * (point_C[1] + point_B[1]);
	double G = A * D - B * C;


	//центр окружности
	double x = (D * E - B * F) / (2 * G);
	double y = (A * F - C * E) / (2 * G);

	double R = sqrt(pow((point_B[0] - x), 2) + pow((point_B[1] - y), 2));

	const double angle_Beta = 2 * 3.1415926 / steps;


	std::vector<double*> vypuk_arr;

	std::vector<std::vector<double>> arr;

	vypuk_arr.push_back(point_A);
	arr.push_back({ point_A[0], point_A[1],point_A[2] });

	for (int i = 0; i <= steps; i++) {
		double new_X = x + R * sin(angle_Beta * i);
		double new_Y = y + R * cos(angle_Beta * i);
		if ((new_X >= point_B[0]) && (new_Y <= point_A[1])) {
			//vypuk_arr.push_back(new double[] {new_X, new_Y, point_A[2]});
			arr.push_back({ new_X, new_Y, point_A[2] });
		}

	}

	vypuk_arr.push_back(point_B);
	arr.push_back({ point_B[0], point_B[1],point_B[2] });

	double point_UP[3];
	double point_UP_1[3];
	double point_UP_2[3];

	//for (int j = 0; j < vypuk_arr.size() - 1; j++) {
	//	point_UP[0] = vypuk_arr[j][0];
	//	point_UP[1] = vypuk_arr[j][1];
	//	point_UP[2] = vypuk_arr[j][2] + height;
	//
	//	glNormal3dv(get_Normal(
	//		vypuk_arr[j],
	//		vypuk_arr[j + 1],
	//		point_UP));
	//
	//	glVertex3dv(vypuk_arr[j]);
	//	glVertex3d(vypuk_arr[j][0], vypuk_arr[j][1], vypuk_arr[j][2] + height);
	//
	//
	//	glVertex3d(vypuk_arr[j + 1][0], vypuk_arr[j + 1][1], vypuk_arr[j + 1][2] + height);
	//	glVertex3dv(vypuk_arr[j + 1]);
	//
	//}

	for (int j = 0; j < int(arr.size()) - 1; j++) {
		point_UP[0] = arr[j][0];
		point_UP[1] = arr[j][1];
		point_UP[2] = arr[j][2] + height;

		point_UP_1[0] = arr[j][0];
		point_UP_1[1] = arr[j][1];
		point_UP_1[2] = arr[j][2];

		point_UP_2[0] = arr[j + 1][0];
		point_UP_2[1] = arr[j + 1][1];
		point_UP_2[2] = arr[j + 1][2];

		glNormal3dv(get_Normal(
			point_UP_1,
			point_UP_2,
			point_UP));

		glVertex3d(arr[j][0], arr[j][1], arr[j][2]);
		glVertex3d(arr[j][0], arr[j][1], arr[j][2] + height);


		glVertex3d(arr[j + 1][0], arr[j + 1][1], arr[j + 1][2] + height);
		glVertex3d(arr[j + 1][0], arr[j + 1][1], arr[j + 1][2]);

	}
}

void draw_Prizm() {

	std::vector<double*> points_arr = {
		new double[] { 0,0,0 },
		new double[] { 1,0,0 },
		new double[] { 1.5,2,0 },
		new double[] { -2,3.5,0 },
		new double[] { -4.5,0.5,0 },
		new double[] { -0.5,0,0 },
		new double[] { -1.5,-2.5,0 },
		new double[] { 0,-0.5,0 },
		new double[] { 3.5,-1.5,0 }
	};
	std::vector<double*> points_arr_upper = {};
	//		new double[] { 0,0,1 },
	//		new double[] { 1,0,1 },
	//		new double[] { 1.5,2,1 },
	//		new double[] { -2,3.5,1 },
	//		new double[] { -4.5,0.5,1 },
	//		new double[] { -0.5,0,1 },
	//		new double[] { -1.5,-2.5,1 },
	//		new double[] { 0,-0.5,1 },
	//		new double[] { 3.5,-1.5,1 }
	//};



	for (int i = 0; i < points_arr.size(); i++) {
		if (i > 0) {
			points_arr[i][0] *= size;
			points_arr[i][1] *= size;
		}
		points_arr_upper.push_back(new double[] {points_arr[i][0], points_arr[i][1], points_arr[i][2] + height});
	}


	glColor4d(0, 0, 0, 1);
	//окружность раз
		glNormal3dv(get_Normal(points_arr[0], points_arr[1], points_arr[2], 1));
		draw_half_Circle(points_arr[2], points_arr[3]);

	//окружность два
		glNormal3dv(get_Normal(points_arr_upper[0], points_arr_upper[1], points_arr_upper[2]));
		draw_half_Circle(points_arr_upper[2], points_arr_upper[3], 1);
	//stena okr

		
		std::vector<double*> vypuk_arr;

	//textura
	
	
	//основа
	glBegin(GL_TRIANGLE_FAN);
		glColor4d(0.5, 0.8, 0.5,1);
		glNormal3dv(get_Normal(points_arr[0], points_arr[1], points_arr[2], 1 ));
		glVertex3dv(points_arr[0]);
		for (int i = 1; i < points_arr.size(); i++) {
			if (i == 4) {
				draw_Vipyk(points_arr[3], points_arr[4]);
			}
			glVertex3dv(points_arr[i]);
		}
		glVertex3dv(points_arr[1]);
	glEnd();


	glBindTexture(GL_TEXTURE_2D, texId);
	//steni
	glBegin(GL_QUADS);
	glColor4d(0.5, 0.5, 0.5, 1);
		for (int i = 1; i < points_arr.size(); i++) {

			///стены впуклости
			if (i == 3){
				draw_Vpyk_Walls(points_arr[3],points_arr[4],height);

				//double point_UP[3];
				//for (int j = 0; j < vypuk_arr.size() - 1; j++) {
				//	point_UP[0] = vypuk_arr[j][0];
				//	point_UP[1] = vypuk_arr[j][1];
				//	point_UP[2] = vypuk_arr[j][2] + height;
				//
				//	glNormal3dv(get_Normal(
				//		vypuk_arr[j],
				//		vypuk_arr[j + 1],
				//		point_UP));
				//
				//	glVertex3dv(vypuk_arr[j]);
				//	glVertex3d(vypuk_arr[j][0], vypuk_arr[j][1], vypuk_arr[j][2] + height);
				//
				//	
				//	glVertex3d(vypuk_arr[j + 1][0], vypuk_arr[j + 1][1], vypuk_arr[j + 1][2] + height);
				//	glVertex3dv(vypuk_arr[j + 1]);
				//	
				//	
				//} 
				
				continue;
			}

			if (!(i + 1 == points_arr.size()))
				glNormal3dv(get_Normal(
					points_arr[i],
					points_arr[i + 1],
					points_arr_upper[i]));
			else
				glNormal3dv(get_Normal(
					points_arr[i],
					points_arr[1],
					points_arr_upper[i]));

			glTexCoord2d(0, 0);
			glVertex3dv(points_arr[i]);
			glTexCoord2d(0, 1);
			glVertex3dv(points_arr_upper[i]);

			if (i + 1 == points_arr.size()) {
				glTexCoord2d(1, 1);
				glVertex3dv(points_arr_upper[1]);
				glTexCoord2d(1, 0);
				glVertex3dv(points_arr[1]);
			}
			else{
				glTexCoord2d(1, 1);
				glVertex3dv(points_arr_upper[i + 1]);
				glTexCoord2d(1, 0);
				glVertex3dv(points_arr[i + 1]);
			}

			
		}
	glEnd();

	glBindTexture(GL_TEXTURE_2D,0);

	//крышка
	//glBegin(GL_TRIANGLE_FAN);
	//	glColor4d(0.6, 0.4, 0.5,0.5);
	//	glNormal3dv(get_Normal(points_arr_upper[0],points_arr_upper[1] ,points_arr_upper[2]));
	//	glVertex3dv(points_arr_upper[0]);
	//	for (int i = 1; i < points_arr_upper.size(); i++) {
	//		if (i == 4) {
	//			draw_Vipyk(points_arr_upper[3], points_arr_upper[4]);
	//		}
	//		glVertex3dv(points_arr_upper[i]);
	//	}
	//	glVertex3dv(points_arr_upper[1]);
	//glEnd();

	//чистим чистим чистим
	for (int i = 0; i < points_arr.size(); i++) {
		delete[] points_arr[i], points_arr_upper[i];
	}
}

GLdouble* get_Normal(double* point_C,double* point_A,double* point_B, bool reverse) {

	double vector_A[3];
	double vector_B[3];

	if (reverse) {
		for (int i = 0; i < 3; i++) {
			vector_A[i] = point_B[i] - point_C[i];
			vector_B[i] = point_A[i] - point_C[i];
		}
	}
	else {
		for (int i = 0; i < 3; i++) {
			vector_A[i] = point_A[i] - point_C[i];
			vector_B[i] = point_B[i] - point_C[i];
		}
	}

	GLdouble vector_Normal[3];

	vector_Normal[0] = vector_A[1] * vector_B[2] - vector_B[1] * vector_A[2];
	vector_Normal[1] = -vector_A[0] * vector_B[2] + vector_B[0] * vector_A[2];
	vector_Normal[2] = vector_A[0] * vector_B[1] - vector_B[0] * vector_A[1];

	double lenght = sqrt(pow(vector_Normal[0], 2) + pow(vector_Normal[1], 2) + pow(vector_Normal[2], 2));

	for (int i = 0; i < 3; i++) {
		vector_Normal[i] /= lenght;
	}

	return vector_Normal;
}

