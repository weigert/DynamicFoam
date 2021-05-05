
//TinyEngine Triangle Base Primitive
struct Triangle: Primitive{
	GLfloat vert[9] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
	Triangle(){
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
		addBuffers(1);
		bind(0, 9, 3, &vert[0]);
		SIZE = 3;
	}
};

/*
================================================================================
													Paramaters and Constants
================================================================================
*/

//Points and Radius
int K = 1000;
float R = 2.0f*sqrt(4.0f/3.14159265f/(float)K);

const float dt = 0.0001f;
float zoom = 1.0f;
float energy = 500.0f;
float scale = 0.2f;
int pathlength = 10;

float pointsize = 4.0f;
float linewidth = 1.0f;

bool equilibriation = true;
bool expansiveflow = false;
bool contractiveflow = false;

int map = 0;

glm::vec3 flowcolor = glm::vec3(0.85, 0.15, 0.15);
glm::vec3 edgecolor = glm::vec3(0.15, 0.85, 0.15);
glm::vec3 meshcolor = glm::vec3(0.35, 0.35, 0.35);

glm::vec3 tricolor0 = glm::vec3(0);
glm::vec3 tricolor1 = glm::vec3(1);

glm::vec3 backcolor = glm::vec3(0);

bool drawmesh = false;
bool drawpoints = true;
bool drawtriangles = false;

/*
================================================================================
												Point and Triangulation System
================================================================================
*/

std::vector<glm::vec2> points;			//Direct Points
std::vector<double> coords;					//Coordinates for Delaunation
std::vector<glm::ivec3> triangles;	//Triangle Point Indexing
std::vector<glm::vec3> trianglecolor;	//Triangle Point Indexing
std::vector<glm::vec2> centers;			//Triangle Circumcenters

void initpoints(){

  //Generate Set of Points, Extract Coordinates
	points.clear();
	coords.clear();

  sample::disc(points, K, glm::vec2(-1), glm::vec2(1));

  for(size_t i = 0; i < points.size(); i++){
    coords.push_back(points[i].x);
    coords.push_back(points[i].y);
  }

}

void deriveproperties(delaunator::Delaunator& d){

  triangles.clear();
	trianglecolor.clear();
  centers.clear();
  for(size_t i = 0; i < d.triangles.size()/3; i++){

		triangles.emplace_back(d.triangles[3*i+2], d.triangles[3*i+1], d.triangles[3*i+0]);

		if(map == 0) //Circumcenter Map
			centers.push_back(tri::circumcenter(points[d.triangles[3*i+0]], points[d.triangles[3*i+1]], points[d.triangles[3*i+2]]));

		if(map == 1) //Barycenter Map
			centers.push_back(tri::barycenter(points[d.triangles[3*i+0]], points[d.triangles[3*i+1]], points[d.triangles[3*i+2]]));

		if(map == 2) //Incenter Map
			centers.push_back(tri::incenter(points[d.triangles[3*i+0]], points[d.triangles[3*i+1]], points[d.triangles[3*i+2]]));

		if(drawtriangles)
			trianglecolor.emplace_back(glm::mix(tricolor0, tricolor1, 10.0f*tri::area(points[d.triangles[3*i+0]], points[d.triangles[3*i+1]], points[d.triangles[3*i+2]])));
		else
			trianglecolor.emplace_back(backcolor);

	}

}

/*
================================================================================
															Particle System
================================================================================
*/

std::vector<int> particles;
std::vector<glm::vec2> ppos;
float pt = 0.0f;
const int Nparticles = 6*K;

int convergence = 5;

void initparticles(delaunator::Delaunator& d){

  particles.clear();
  ppos.clear();

  for(int i = 0; i < Nparticles; i++){
    int index = rand()%(d.triangles.size());
    while(d.halfedges[index] == -1 || d.halfedges[index] >= d.triangles.size())
      index = rand()%(d.triangles.size());
    particles.push_back(index);
  }

  //Compute the Particles Positions!
  for(size_t i = 0; i < particles.size(); i++)
    ppos.push_back(centers[particles[i]/3]);

}

bool nextpos(delaunator::Delaunator& d, int& pos){

	int t0 = pos/3;									//Triangle

	int e1 = d.halfedges[pos];		  	//Opposite Side Half-Edge
	int t1 = d.halfedges[pos]/3;			//Next Triangle

	if(t1 > d.triangles.size())				//Not Triangle
		return pos;

	int eA = (e1%3 == 2)?e1-2:e1+1;		//Outward Edges of next Triangle
	int eB = (e1%3 == 0)?e1+2:e1-1;

	//Check for Negative Values
	if(d.halfedges[eA] < 0 || d.halfedges[eA] >= d.triangles.size()){
		if(d.halfedges[eB] >= 0 && d.halfedges[eB] < d.triangles.size())
			pos = eB;
		return false;
	}
	if(d.halfedges[eB] < 0 || d.halfedges[eB] >= d.triangles.size()){
		if(d.halfedges[eA] >= 0 && d.halfedges[eA] < d.triangles.size())
			pos = eA;
		return false;
	}

	int tA = d.halfedges[eA]/3;							//Triangle on Other Side
	int tB = d.halfedges[eB]/3;							//Triangle on Other Side

	glm::vec2 dir = glm::normalize(centers[t1] - centers[t0]);
	glm::vec2 dirA = glm::normalize(centers[tA] - centers[t1]);
	glm::vec2 dirB = glm::normalize(centers[tB] - centers[t1]);

	//Direct Select
	if(glm::dot(dir, dirA) > glm::dot(dir, dirB)) pos = eA;
	else pos = eB;

	return true;

}

void updateparticles(delaunator::Delaunator d){

  //Branching Rule
  for(size_t i = 0; i < particles.size(); i++)
		nextpos(d, particles[i]);

}

void particlepos(delaunator::Delaunator d){

  for(size_t i = 0; i < ppos.size(); i++){

    int t0 = particles[i]/3;														//First Triangle
    int t1 = d.halfedges[particles[i]]/3;								//Next Triangle
    ppos[i] = glm::mix(centers[t0], centers[t1], pt);

  }

}

/*
================================================================================
                              Coloring Schemes
================================================================================

Idea: For every triangle (i.e. point + direction), we can compute two quantities:
- The distance to the nearest stable orbit
- The amount of catchment flowing through the position

*/

std::vector<float> catchment;

void accumulate(delaunator::Delaunator d){

  catchment.clear();

  const size_t halfedgesize = d.triangles.size();
  catchment.resize(halfedgesize);

	for(int i = 0; i < halfedgesize; i++){
		catchment[i] = 0;
	}

  //Compute the Orbit Distance for every Half-Edge
  for(int i = 0; i < halfedgesize; i++){

    //Boolean Visited Array
//    bool visited[halfedgesize] = {false};
    int curpos = i;

		for(size_t i = 0; i < convergence; i++){
			nextpos(d, curpos);
			catchment[curpos]++;
		}

  }

}


/*
================================================================================
                              Model Constructors
================================================================================
*/

std::function<void(Model* m, std::vector<glm::vec2>, glm::vec3)> pointconstruct = [](Model* m, std::vector<glm::vec2> pvec, glm::vec3 color){
  for(auto& p: pvec){
    m->positions.push_back(p.x);
    m->positions.push_back(p.y);
    m->positions.push_back(0.0f);

    m->normals.push_back(color.x);
    m->normals.push_back(color.y);
    m->normals.push_back(color.z);
  }
};


std::function<void(Model*, delaunator::Delaunator)> triangleconstruct = [](Model* m, delaunator::Delaunator d){

    for(size_t i = 0; i < d.triangles.size()/3; i++){
      m->positions.push_back(centers[i].x);
      m->positions.push_back(centers[i].y);
      m->positions.push_back(0.0f);

			m->normals.push_back(meshcolor.x);
			m->normals.push_back(meshcolor.y);
			m->normals.push_back(meshcolor.z);

      //For each triangle there is exactly one point here
      //But for each triangle there are three possible lines

      //From this guy...
      if(d.halfedges[3*i+0] != -1){
        m->indices.push_back(i);
        m->indices.push_back(d.halfedges[3*i+0]/3);
    //		std::cout<<d.halfedges[3*i+0]<<std::endl;
      }
      if(d.halfedges[3*i+1] != -1){
        m->indices.push_back(i);
        m->indices.push_back(d.halfedges[3*i+1]/3);
      }
      if(d.halfedges[3*i+2] != -1){
        m->indices.push_back(i);
        m->indices.push_back(d.halfedges[3*i+2]/3);
      }

    }
};


std::function<void(Model* m)> particleconstructor = [](Model* m){
  for(auto& p: ppos){
    m->positions.push_back(p.x);
    m->positions.push_back(p.y);
    m->positions.push_back(0.0f);

    m->normals.push_back(1.0f);
    m->normals.push_back(0.0f);
    m->normals.push_back(0.0f);
  }
};
