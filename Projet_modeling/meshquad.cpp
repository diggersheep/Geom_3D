#include "meshquad.h"
#include "matrices.h"

#include <unistd.h>

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
	return m_points.size();
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
		if (
			   ( i1 == edges[offset] || i1 == edges[offset+1] )
			&& ( i2 == edges[offset] || i2 == edges[offset+1] )
			)
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
	int size = edges.size() / 2;
	for ( int i = 0 ; i < ( size - 1)  ; i++ ) // size -1 car le dernier quads n'a que des arrêtes en commun
	{
		int offset = i * 2;
		// 1er arrête 0-1
		if ( this->borrowed_edges(quads[offset], quads[offset+1], edges) )
		{
			edges.push_back(quads[offset  ]);
			edges.push_back(quads[offset+1]);
		}
		// 2nd arrête 1-2
		if ( this->borrowed_edges(quads[offset+1], quads[offset+2], edges) )
		{
			edges.push_back(quads[offset+1]);
			edges.push_back(quads[offset+2]);
		}
		// 3e arrête 2-3
		if ( this->borrowed_edges(quads[offset+2], quads[offset+3], edges) )
		{
			edges.push_back(quads[offset+2]);
			edges.push_back(quads[offset+3]);
		}
		// 4e arrête 3-0
		if ( this->borrowed_edges(quads[offset+3], quads[offset], edges) )
		{
			edges.push_back(quads[offset+3]);
			edges.push_back(quads[offset  ]);
		}
	}
}


void MeshQuad::create_cube()
{
	clear();
	// ajouter 8 sommets (-1 +1)

	double coef = 2; //taille initiale du cube

	for ( int x = 0 ; x < 2 ; x++ )
	{
		for ( int y = 0 ; y < 2 ; y++)
		{
			for ( int z = 0 ; z < 2 ; z++ )
			{
				this->add_vertex(Vec3(x*coef, y*coef, z*coef));
			}
		}
	}
			
	// ajouter 6 faces (sens trigo)

	this->add_quad(6,2,0,4);
	this->add_quad(0,1,5,4);

	this->add_quad(3,7,5,1);
	this->add_quad(6,4,5,7);

	this->add_quad(2,6,7,3);
	this->add_quad(0,2,3,1);
	gl_update();
/*
	Vec3 a(0,1,0);
	Vec3 b(0,0,0);
	Vec3 c(1,0,0);
	Vec3 d(1,1,0);
	Vec3 p(0.25,0.75,0);

	this->is_points_in_quad(p, a, b, c, d);
*/
}

Vec3 MeshQuad::is_sparta ( void ) { return Vec3(); }
Vec3 MeshQuad::normal_of_quad(const Vec3& A, const Vec3& B, const Vec3& C, const Vec3& D)
{
	// Attention a l'ordre des points !
	// le produit vectoriel n'est pas commutatif U ^ V = - V ^ U
	// ne pas oublier de normaliser le resultat.

	Vec3 AB = Vec3( B[0] - A[0], B[1] - A[1], B[2] - A[2] );
	Vec3 BC = Vec3( C[0] - B[0], C[1] - B[1], C[2] - B[2] );
	Vec3 CD = Vec3( D[0] - C[0], D[1] - C[1], D[2] - C[2] );
	Vec3 DA = Vec3( A[0] - D[0], A[1] - D[1], A[2] - D[2] );

	Vec3 n = Vec3();

	n += this->is_sparta();
	n += glm::cross(AB, BC);
	n += glm::cross(BC, CD);
	n += glm::cross(CD, DA);
	n += glm::cross(DA, AB);

//	std::cout << "avant normalisation " << n[0] << " " << n[1] << " " << n[2] << std::endl;
	
	for ( int i = 0 ; i < 3 ; i++)
		n[i] /= 4.0;

	n = glm::normalize(n);
//	std::cout << "après normalisation " << n[0] << " " << n[1] << " " << n[2] << std::endl;

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
{/*
	// On sait que P est dans le plan du quad.

	// est dans le triangle ABC
	if (
		glm::dot( glm::cross((B-A), (P-A)), glm::cross((P-A), (C-A))) >= 0 && // AB^AP * AP^AC
		glm::dot( glm::cross((A-B), (P-B)), glm::cross((P-B), (C-B))) >= 0 && // BA^BP * BP^BC
		glm::dot( glm::cross((A-C), (P-C)), glm::cross((P-C), (B-C))) >= 0    // CA^CP * CP^CB
	)
	{
		return true;
	}

	// est dans le triangle ACD
	if (
		glm::dot( glm::cross((B - A), (P-A)), glm::cross((P-A), (D-A))) >= 0 && // AB^AP * AP^AD
		glm::dot( glm::cross((A-C), (P-C)), glm::cross((P-C), (D-C))) >= 0 &&   // CA^CP * CP^CD
		glm::dot( glm::cross((A-D), (P-D)), glm::cross((P-D), (C-D))) >= 0      // DA^DP * DP^DC
	)
	{
		return true;
	}*/

	// P est-il au dessus des 4 plans contenant chacun la normale au quad et une arete AB/BC/CD/DA ?
	// si oui il est dans le quad
	//      nope, c'est un quadrilataire pas un polyhèdre

	return false;
}

bool MeshQuad::intersect_ray_quad(const Vec3& P, const Vec3& Dir, int q, Vec3& inter)
{
	/*
	if ( q < 0 )
		return false;
	if ( static_cast<unsigned int>(q * 4) >= this->m_quad_indices.size() )
		return false;

	// recuperation des indices de points
	// recuperation des points
	float d;
	float t;
	Vec3  I;
	Vec3 A = this->m_points[ this->m_quad_indices[4*q+0] ];
	Vec3 B = this->m_points[ this->m_quad_indices[4*q+1] ];
	Vec3 C = this->m_points[ this->m_quad_indices[4*q+2] ];
	Vec3 D = this->m_points[ this->m_quad_indices[4*q+3] ];

	Vec3 n  = this->normal_of_quad(A, B, C, D); // normale au plan
	Vec3 Q  = P + Dir; // point Q, extémimité du rayon PQ
	Vec3 PQ = Q - P; // 
	d       = glm::length(A); //

	// calcul de l'intersection rayon plan
	// I = P + alpha*Dir est dans le plan => calcul de alpha
	// alpha => calcul de I

	t = ( d - glm::dot( Q - P , n) ) / ( glm::dot( PQ, n ) );

	if ( 0 >= t || t >= 1 )
	{
		std::cout << "Pas dans le segment PQ" << std::endl;
		return false;
	}

	I = P + t * Dir;
/*
	// I dans le quad ?
	if ( this->is_points_in_quad(I, A, B, C, D) )
	{
		inter = I;
		return true;
	}*/

	return false;
}


int MeshQuad::intersected_visible(const Vec3& P, const Vec3& Dir)
{
	// on parcours tous les quads
	int inter = -1;
	
		// on teste si il y a intersection avec le rayon
		// on garde le plus proche (de P)

	return inter;
}


Mat4 MeshQuad::local_frame(int q)
{
	// Repere locale = Matrice de transfo avec
	// les trois premieres colones: X,Y,Z locaux
	// la derniere colonne l'origine du repere

	// ici Z = N et X = AB
	// Origine le centre de la face
	// longueur des axes : [AB]/2

	// recuperation des indices de points
	// recuperation des points

	// calcul de Z:N puis de X:arete on en deduit Y

	// calcul du centre

	// calcul de la taille

	// calcul de la matrice

	return Mat4();
}

void MeshQuad::extrude_quad(int q)
{
	// recuperation des indices de points

	// recuperation des points

	// calcul de la normale

	// calcul de la hauteur

	// calcul et ajout des 4 nouveaux points

	// on remplace le quad initial par le quad du dessu

	// on ajoute les 4 quads des cotes

	gl_update();
}


void MeshQuad::decale_quad(int q, float d)
{
	// recuperation des indices de points

	// recuperation des (references de) points

	// calcul de la normale

	// modification des points

	gl_update();
}


void MeshQuad::shrink_quad(int q, float s)
{
	// recuperation des indices de points

	// recuperation des (references de) points

	// ici  pas besoin de passer par une matrice
	// calcul du centre

	 // modification des points

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

