#include "meshquad.h"
#include "matrices.h"
#include "viewer.h"

#include <unistd.h>


/**/
float rad_to_deg ( float rad )
{
	return rad * 360 / (float)(2 * M_PI );
}
int sign ( float x )
{
	if ( x == 0 )
		return 0;
	else if ( x < 0 )
		return -1;
	else if ( x > 0 )
		return 1;
}
/**/


MeshQuad::MeshQuad():
	m_nb_ind_edges(0)
{

}


void MeshQuad::gl_init()
{
	m_shader_flat = new ShaderProgramFlat();
	m_shader_color = new ShaderProgramColor();

	//VBO
	glGenBuffers(1, &m_vbo);

	//VAO
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glEnableVertexAttribArray(m_shader_flat->idOfVertexAttribute);
	glVertexAttribPointer(m_shader_flat->idOfVertexAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindVertexArray(0);

	glGenVertexArrays(1, &m_vao2);
	glBindVertexArray(m_vao2);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glEnableVertexAttribArray(m_shader_color->idOfVertexAttribute);
	glVertexAttribPointer(m_shader_color->idOfVertexAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindVertexArray(0);


	//EBO indices
	glGenBuffers(1, &m_ebo);
	glGenBuffers(1, &m_ebo2);
}

void MeshQuad::gl_update()
{
	//VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * m_points.size() * sizeof(GLfloat), &(m_points[0][0]), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	std::vector<int> tri_indices;
	convert_quads_to_tris(m_quad_indices,tri_indices);

	//EBO indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,tri_indices.size() * sizeof(int), &(tri_indices[0]), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	std::vector<int> edge_indices;
	convert_quads_to_edges(m_quad_indices,edge_indices);
	m_nb_ind_edges = edge_indices.size();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,m_nb_ind_edges * sizeof(int), &(edge_indices[0]), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}



void MeshQuad::set_matrices(const Mat4& view, const Mat4& projection)
{
	viewMatrix = view;
	projectionMatrix = projection;
}

void MeshQuad::draw(const Vec3& color)
{

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(1.0f, 1.0f);

	m_shader_flat->startUseProgram();
	m_shader_flat->sendViewMatrix(viewMatrix);
	m_shader_flat->sendProjectionMatrix(projectionMatrix);
	glUniform3fv(m_shader_flat->idOfColorUniform, 1, glm::value_ptr(color));
	glBindVertexArray(m_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_ebo);
	glDrawElements(GL_TRIANGLES, 3*m_quad_indices.size()/2,GL_UNSIGNED_INT,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	glBindVertexArray(0);
	m_shader_flat->stopUseProgram();

	glDisable(GL_POLYGON_OFFSET_FILL);

	m_shader_color->startUseProgram();
	m_shader_color->sendViewMatrix(viewMatrix);
	m_shader_color->sendProjectionMatrix(projectionMatrix);
	glUniform3f(m_shader_color->idOfColorUniform, 0.0f,0.0f,0.0f);
	glBindVertexArray(m_vao2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m_ebo2);
	glDrawElements(GL_LINES, m_nb_ind_edges,GL_UNSIGNED_INT,0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
	glBindVertexArray(0);
	m_shader_color->stopUseProgram();
}

void MeshQuad::clear()
{
	m_points.clear();
	m_quad_indices.clear();
}

int MeshQuad::add_vertex(const Vec3& P)
{
	m_points.push_back( P );
	return m_points.size() - 1;
}


void MeshQuad::add_quad(int i1, int i2, int i3, int i4)
{
	int size = m_points.size();

	// condition
	// cohérence des indices
	if ( 0 > i1 && i1 > size ) return;
	if ( 0 > i2 && i2 > size ) return;
	if ( 0 > i3 && i3 > size ) return;
	if ( 0 > i4 && i4 > size ) return;

	// non égalité des indices
	if ( i1 == i2 || i1 == i3 || i1 == i4 || i2 == i3 || i2 == i4 || i3 == i4 ) return;

	m_quad_indices.push_back(i1);
	m_quad_indices.push_back(i2);
	m_quad_indices.push_back(i3);
	m_quad_indices.push_back(i4);
}

void MeshQuad::convert_quads_to_tris(const std::vector<int>& quads, std::vector<int>& tris)
{
	tris.clear();
	tris.reserve(3*quads.size()/2); // 1 quad = 4 indices -> 2 tris = 6 indices d'ou ce calcul (attention division entiere)

	// Pour chaque quad on genere 2 triangles
	// Attention a respecter l'orientation des triangles
	
	/*es-ce qu'on check pour les polygones canvaves ??*/

	int size = quads.size() / 4;
	for ( int i = 0 ; i < size ; i++ )
	{
		int j = i * 4;

		// 1er triangle
		tris.push_back( quads[j + 0] );
		tris.push_back( quads[j + 2] );
		tris.push_back( quads[j + 1] );
		
		// 2e triangle
		tris.push_back( quads[j + 0] );
		tris.push_back( quads[j + 3] );
		tris.push_back( quads[j + 2] );
	}
}

bool MeshQuad::borrowed_edges (int i1, int i2, const std::vector<int>& edges)
{
	int size = edges.size() / 2;

	for ( int i = 0 ; i < size ; i++ )
	{
		int offset = i * 2;
		// c'est moche comme condition, mais bon ...
		if ( i1 == edges[offset] && i1 == edges[offset+1] )
		{
			return true;
		}
		else if ( i2 == edges[offset] && i2 == edges[offset+1] )
		{
			return true; 
		}
	}

	return false;
}
void MeshQuad::convert_quads_to_edges(const std::vector<int>& quads, std::vector<int>& edges)
{
	// complexité O( n*m )

	edges.clear();
	edges.reserve(quads.size()); // ( *2 /2 !)

	// Pour chaque quad on genere 4 aretes, 1 arete = 2 indices.
	// Mais chaque arete est commune a 2 quads voisins !
	
	// Comment n'avoir qu'une seule fois chaque arete ?
	// -> check du tableau d'arrêtes
	int size_q = quads.size() / 4; 
	for ( int i = 0 ; i < size_q ; i++ )
	{
		float a = quads[4*i + 0];
		float b = quads[4*i + 1];
		float c = quads[4*i + 2];
		float d = quads[4*i + 3];
	
		if ( !this->borrowed_edges( a, b, edges ) )
		{
			edges.push_back(a);
			edges.push_back(b);
		}
		if ( !this->borrowed_edges( b, c, edges ) )
		{
			edges.push_back(b);
			edges.push_back(c);
		}
		if ( !this->borrowed_edges( c, d, edges ) )
		{
			edges.push_back(c);
			edges.push_back(d);
		}
		if ( !this->borrowed_edges( d, a, edges ) )
		{
			edges.push_back(d);
			edges.push_back(a);
		}
	}
}


void MeshQuad::create_cube()
{
	clear();
	// ajouter 8 sommets (-1 +1)

	double coef = 2; //taille initiale du cube

	this->add_vertex( Vec3(0*coef, 0*coef, 0*coef));
	this->add_vertex( Vec3(1*coef, 0*coef, 0*coef));
	this->add_vertex( Vec3(1*coef, 1*coef, 0*coef));
	this->add_vertex( Vec3(0*coef, 1*coef, 0*coef));

	this->add_vertex( Vec3(0*coef, 1*coef, 1*coef));
	this->add_vertex( Vec3(1*coef, 1*coef, 1*coef));
	this->add_vertex( Vec3(1*coef, 0*coef, 1*coef));
	this->add_vertex( Vec3(0*coef, 0*coef, 1*coef));
			
	// ajouter 6 faces (sens trigo)

	this->add_quad(0,1,2,3);
	this->add_quad(2,1,6,5);
	this->add_quad(4,5,6,7);
	this->add_quad(1,0,7,6);
	this->add_quad(0,3,4,7);
	this->add_quad(3,2,5,4);



	Vec3 A(0,0,0);
	Vec3 B(1,0,0);
	Vec3 C(1,1,0);
	Vec3 D(0,1,0);
	Vec3 P(0.5, 10.5, 0.0001);

	bool a = is_points_in_quad( P, A, B, C, D );

	if ( a )
		std::cout << "le point est dedans" << std::endl;
	else
		std::cout << "le point n'est pas dedans" << std::endl;

	gl_update();

}

Vec3 MeshQuad::is_sparta ( void ) { return Vec3(); }
Vec3 MeshQuad::normal_of_quad(const Vec3& A, const Vec3& B, const Vec3& C, const Vec3& D)
{
	// Attention a l'ordre des points !
	// le produit vectoriel n'est pas commutatif U ^ V = - V ^ U
	// ne pas oublier de normaliser le resultat.

	Vec3 AB = B - A;
	Vec3 BC = C - B;
	Vec3 CD = D - C;
	Vec3 DA = A - D;

	Vec3 n = Vec3();

	n += this->is_sparta(); // 300
	n += glm::cross(BC, AB);
	n += glm::cross(CD, BC);
	n += glm::cross(DA, CD);
	n += glm::cross(AB, DA);

	// pour faire la moyenne des normales (non unitaires)
	n.x /= 4.0;
	n.y /= 4.0;
	n.z /= 4.0;

	n = glm::normalize(n);

	return n;
}

float MeshQuad::area_of_quad(const Vec3& A, const Vec3& B, const Vec3& C, const Vec3& D)
{
	// Théorème de Varignon
	Vec3 I = Vec3( (A.x + B.x) / 2, (A.y + B.y) / 2, (A.z + B.z) / 2 );
	Vec3 J = Vec3( (B.x + C.x) / 2, (B.y + C.y) / 2, (B.z + C.z) / 2 );
	Vec3 K = Vec3( (C.x + D.x) / 2, (C.y + D.y) / 2, (C.z + D.z) / 2 );

	// 
	Vec3 JI = Vec3( I.x - J.x, I.y - J.y, I.z - J.z );
	Vec3 IK = Vec3( K.x - I.x, K.y - I.y, K.z - I.z );

	glm::mat3 m = glm::mat3(IK, JI, Vec3(1,1,1));

	return abs(glm::determinant(m)) * 2;
}



bool MeshQuad::is_points_in_quad(const Vec3& P, const Vec3& A, const Vec3& B, const Vec3& C, const Vec3& D)
{
	// On sait que P est dans le plan du quad.

	// est dans le triangle ABC
/*
	if (
		glm::dot( glm::cross((B-A), (P-A)), glm::cross((P-A), (C-A))) >= 0 && // AB^AP * AP^AC
		glm::dot( glm::cross((A-B), (P-B)), glm::cross((P-B), (C-B))) >= 0 && // BA^BP * BP^BC
		glm::dot( glm::cross((A-C), (P-C)), glm::cross((P-C), (B-C))) >= 0    // CA^CP * CP^CB
	)
	{
		std::cout << "  ok1" << std::endl;
		return true;
	}

	// est dans le triangle ACD
	if (
		glm::dot( glm::cross((B - A), (P-A)), glm::cross((P-A), (D-A))) >= 0 && // AB^AP * AP^AD
		glm::dot( glm::cross((A-C), (P-C)), glm::cross((P-C), (D-C))) >= 0 &&   // CA^CP * CP^CD
		glm::dot( glm::cross((A-D), (P-D)), glm::cross((P-D), (C-D))) >= 0      // DA^DP * DP^DC
	)
	{
		std::cout << "  ok2" << std::endl;
		return true;
	}
*/
	// P est-il au dessus des 4 plans contenant chacun la normale au quad et une arete AB/BC/CD/DA ?
	// si oui il est dans le quad
	//      nope, c'est un quadrilataire pas un polyhèdre

	float algebric_distance[4];
	std::vector<Vec3> quad;
	quad.push_back(A);
	quad.push_back(B);
	quad.push_back(C);
	quad.push_back(D);

	Vec3 n = this->normal_of_quad(A, B, C, D);
	std::cout << "point P " << P << std::endl;

	for ( int i = 0 ; i < 4 ; i++ )
	{
		Vec3 I1 = quad[i];

		Vec3 I2 = quad[(i+1) % 4];

		float p   = glm::length(I1);
		float tmp = glm::length(I2);
		if ( tmp < p ) p = tmp;

//		Vec3 n_bis = glm::cross( (I2 - I1) , n );
		Vec3 n_bis = glm::cross( n, (I2 - I1) );

		tmp = glm::dot( n, n_bis ) - p;
		float tmp2 = glm::dot( n, glm::cross( n, (I2 - I1) ) ) - p;
	
		std::cout << "normale " << i << " = " << n_bis << std::endl;	
		std::cout << "dist alg " << i << " = " << tmp << " | " << tmp2 << std::endl;

		if ( sign(tmp) == sign(1) )
		{
			std::cout << "fail sign " << i << std::endl;
			return false;
		}
	//	algebric_distance[i] = ( tmp );
	}

	/*
	for ( int i = 0 ; i < 4 ; i++ )
	{
		int j = i - 1;
		if ( j < 0 ) j = 3;

		int s1 = sign( algebric_distance[i] );
		int s2 = sign( algebric_distance[j] );
	}
	*/

	std::cout << "signe ok" << std::endl;
	return true;
}

bool MeshQuad::intersect_ray_quad(const Vec3& P, const Vec3& Dir, int q, Vec3& inter)
{
	if ( q < 0 )
		return false;
	if ( static_cast<unsigned int>(q * 4) >= this->m_quad_indices.size() )
		return false;

	float precision = 0.0000001;

	// recuperation des indices de points
	// recuperation des points
	float d;
	float t;
	float tmp;
	Vec3  I;

	Vec3 A = this->m_points[ this->m_quad_indices[4*q+0] ];
	Vec3 B = this->m_points[ this->m_quad_indices[4*q+1] ];
	Vec3 C = this->m_points[ this->m_quad_indices[4*q+2] ];
	Vec3 D = this->m_points[ this->m_quad_indices[4*q+3] ];


	Vec3 n = this->normal_of_quad(A, B, C, D); // normale au plan

	if (  abs(glm::dot( n, Dir )) < precision )
	{
	//	std::cout << "i fail (précision)" << std::endl;
		return false;
	}

	// prend la distance minumum (c'est moche mais ça fonctionne super bien !)
	d = glm::length(A);

	tmp = glm::length(B);
	if ( tmp < d ) d = tmp;
	tmp = glm::length(C);
	if ( tmp < d ) d = tmp;
	tmp = glm::length(C);
	if ( tmp < d ) d = tmp;

	if ( abs(glm::dot(n, Dir)) < precision )
	{
		return false;
	}

	/*
	paramètre t du rayon (P.Dir)

			 d - P . n
		t = -----------
			 Dir . n
	*/
	float tmp1 = (d - glm::dot(P, n));
	float tmp2 = (glm::dot(n, Dir));
	t =  tmp1 / tmp2;

	I = P + t * Dir;

	/*	
	std::cout << "t : " << tmp1 << " / " << tmp2 << " = " << t << std::endl;
	std::cout << "Normale : " << n << " || I : " << I << std::endl;
	std::cout << "    A : " << A << std::endl;
	std::cout << "    B : " << B << std::endl;
	std::cout << "    C : " << C << std::endl;
	std::cout << "    D : " << D << std::endl;
	*/

	if ( this->is_points_in_quad( I, A, B, C, D ) )
	{
	//	std::cout << "i OKKKKK " << q <<  std::endl;
		inter = I;
		return true;
	}

	//std::cout << "i fail " << q <<  std::endl;
	return false;
}


int MeshQuad::intersected_visible(const Vec3& P, const Vec3& Dir)
{
	int   inter    = -1;
	float distance = -1;
	Vec3  I;

	int n = 0;

	std::cout << std::endl;

	std::cout << " P => " << P << std::endl;
	
	int size = this->m_quad_indices.size() / 4;
	for ( int i = 0 ; i < size ; i++ )
	{
	//	std::cout << "(" << (i+1) << "/" << size << ")" << std::endl;
		if ( intersect_ray_quad(P, Dir, i, I) )
		{
			n++;
			float tmp = glm::distance(P, I);
			std::cout << "    face #" << i << " -> distance = " << tmp << " | " << I << std::endl;
			if ( distance < 0 )
			{
				distance = tmp;
				inter = i;
			}
			else if ( tmp <= distance )
			{
				distance = tmp;
				inter    = i;
			}
		}	
	}
	
	std::cout << "Face :: " << inter << std::endl;
	std::cout << "Nombre d'intersection " << n << std::endl;
	return inter;
}

Mat4 MeshQuad::local_frame(int q)
{

	// test s'il c'est dans le tableau
	if ( q < 0 || q >= static_cast<int>(this->m_quad_indices.size() / 4) )
		return Mat4();

	// Repere locale = Matrice de transfo avec
	// les trois premieres colones: X,Y,Z locaux
	// la derniere colonne l'origine du repere

	float size; // scale
	Vec3 M;  // milieu du quad
	Vec3 n;  // normale du quad
	Vec3 Y;
	Vec3 AB; // vecteur AB 
	Mat4 t = Mat4(); // matrice de transformation 3D

	Vec3 A = this->m_points[this->m_quad_indices[4*q + 0]];
	Vec3 B = this->m_points[this->m_quad_indices[4*q + 1]];
	Vec3 C = this->m_points[this->m_quad_indices[4*q + 2]];
	Vec3 D = this->m_points[this->m_quad_indices[4*q + 3]];

	// ici Z = N et X = AB
	n  = this->normal_of_quad(A, B, C, D);
	AB = B - A;
	Y  = glm::cross(n, AB);

	// Origine le centre de la face
	M = (A + B + C + D);
	M.x /= 4;
	M.y /= 4;
	M.z /= 4;

	// longueur des axes : [AB]/2
	size = glm::length(B - A) / 2;

	// recuperation des indices de points
	// recuperation des points

	// calcul de Z:N puis de X:arete on en deduit Y

	// calcul du centre

	// calcul de la taille

	// calcul de la matrice


	t *= translate( M.x, M.y, M.z );
	t *= scale( size, size, size );

	return t;
}

void MeshQuad::extrude_quad(int q)
{
	float height;
	float coef = 1;
	Vec3 t;
	Vec3 n;

	float i_points[4];
	float new_i_points[4];
	Vec3  points[4];
	Vec3  new_points[4];

	// recuperation des indices de points
	// recuperation des points
	for ( int i = 0 ; i  < 4 ; i++ )
	{
		i_points[i] = this->m_quad_indices[q*4 + i];
		points[i]   = this->m_points[i_points[i]];
	}


	// calcul de la normale
	n = this->normal_of_quad(points[0], points[1], points[2], points[3]);
	
	// calcul de la hauteur
	height = sqrt( this->area_of_quad(points[0], points[1], points[2], points[3]) ) * coef;
	
	// calcul et ajout des 4 nouveaux points
	for ( int i = 0 ; i < 4 ; i++ )
	{
		new_points[i] = points[i] + (n * height);
		new_i_points[i] = this->add_vertex(new_points[i]);

		// on remplace le quad initial par le quad du dessu
		this->m_quad_indices[q*4 + i] = new_i_points[i];
	
		std::cout << "new point (i:"  << new_i_points[i] << ") : " << new_points[i] << std::endl;
	}

	// on ajoute les 4 quads des cotes
	for ( int i = 0 ; i < 4 ; i++ )
	{
		int j = (i+1) % 4;
		this->add_quad( i_points[i], i_points[j], new_i_points[j], new_i_points[i] );	
	}

	gl_update();
}


void MeshQuad::decale_quad(int q, float d)
{
	float height;
	Vec3 t;
	Vec3 n;

	float i_points[4];
	Vec3  points[4];
	// recuperation des indices de points	
	// recuperation des (references de) points
	for ( int i = 0 ; i  < 4 ; i++ )
	{
		i_points[i] = this->m_quad_indices[q*4 + i];
		points[i]   = this->m_points[i_points[i]];
	}

	// calcul de la normale
	n = this->normal_of_quad(
		points[0],
		points[1],
		points[2],
		points[3]
	);
	height = sqrt( this->area_of_quad(points[0], points[1], points[2], points[3]) ) * d;
	t      = n * height;

	// modification des points
	// calcul et ajout des 4 nouveaux points
	for ( int i = 0 ; i < 4 ; i++ )
	{
		this->m_points[i_points[i]] += t;
	}

	gl_update();
}


void MeshQuad::shrink_quad(int q, float s)
{
	int  i_points[4];
	Vec3 points[4];
	Vec3 M;

	// recuperation des indices de points
	// recuperation des (references de) points
	for ( int i = 0 ; i  < 4 ; i++ )
	{
		i_points[i] = this->m_quad_indices[q*4 + i];
		points[i]   = this->m_points[i_points[i]];
	}

	s -= 1;

	// ici  pas besoin de passer par une matrice
	// calcul du centre
	M = (points[0] + points[1] + points[2] + points[3]);
	M.x /= 4;
	M.y /= 4;
	M.z /= 4;

	// modification des points
	for ( int i = 0 ; i < 4 ; i++ )
		this->m_points[i_points[i]] -= (M - points[i]) * s;

	std::cout << "scale " << s + 1 << std::endl;
	gl_update();
}


void MeshQuad::tourne_quad(int q, float a)
{
	// recuperation des indices de points

	// recuperation des (references de) points

	// generation de la matrice de transfo:
	// tourne autour du Z de la local frame
	// indice utilisation de glm::inverse()

	// Application au 4 points du quad


	gl_update();
}

