#include <TinyEngine/TinyEngine>

#include "include/poisson.h"
#include "include/triangle.h"
#include "include/delaunator-cpp/delaunator-header-only.hpp"
#include "include/model.h"

int main( int argc, char* args[] ) {

	//Initialize the System

	initpoints();													//Sample Points
  delaunator::Delaunator d(coords);			//Compute Delaunay Triangulation
	deriveproperties(d);									//Compute Derived Properties
	initparticles(d);											//Setup Particle System
	accumulate(d);												//Perform Flow Accumulation

	//Initialize a Window
	Tiny::view.pointSize = 4.0f;
	Tiny::view.vsync = false;
	Tiny::window("Example Window", 1000, 1000);

	//Triangles Rendering
	Triangle triangle;
	Instance triangleinstance(&triangle);
	triangleinstance.addBuffer(triangles);

	//Create the Shaders
	Shader pointshader({"shader/point.vs", "shader/point.fs"}, {"in_Position", "in_Color"});
	Shader triangleshader({"shader/triangle.vs", "shader/triangle.fs"}, {"in_Position", "in_Index", "in_Orbit"}, {"points"});
	triangleshader.buffer("points", points);

	//Rendering Models
	Model particlemodel(pointconstruct, ppos, flowcolor);
	particlemodel.indexed = false;

	Model pointmodel(pointconstruct, points, edgecolor);
	pointmodel.indexed = false;

	Model centermodel(triangleconstruct, d);

	bool paused = true;
	Tiny::event.handler = [&](){
		if(!Tiny::event.press.empty() && Tiny::event.press.back() == SDLK_p){
			paused = !paused;
		}
	};

	Tiny::view.interface = [&](){

		if(ImGui::BeginTabBar("Tab Bar", ImGuiTabBarFlags_None)){

		if(ImGui::BeginTabItem("Point Set")){
			ImGui::DragInt("N-Points", &K, 10, 10, 2000);
			if(ImGui::Button("Re-Seed")){
				initpoints();
				d.compute(coords);
				deriveproperties(d);
				accumulate(d);
			}
			ImGui::EndTabItem();
		}

		if(ImGui::BeginTabItem("Dynamics")){

			ImGui::DragFloat("Energy", &energy, 10.0f, 0.0f, 5000.0f);
			ImGui::DragFloat("Scale", &scale, 0.01f, 0.01f, 1.0f);
			ImGui::DragInt("Criticality", &convergence, 1, 1, 100);

			ImGui::RadioButton("Circumcenter", &map, 0); ImGui::SameLine();
			ImGui::RadioButton("Barycenter", &map, 1);
			ImGui::RadioButton("Incenter", &map, 2);

			ImGui::Checkbox("Equilibriation", &equilibriation);
			ImGui::Checkbox("Expansive Flow", &expansiveflow);	ImGui::SameLine();
			ImGui::Checkbox("Contractive Flow", &contractiveflow);

			ImGui::EndTabItem();

		}

		if(ImGui::BeginTabItem("Visualization")){

			ImGui::DragInt("Path Steps", &pathlength, 1, 1, 100);
			ImGui::Checkbox("Draw Mesh", &drawmesh);
			ImGui::ColorEdit3("Mesh Color", &meshcolor[0]);

			ImGui::ColorEdit3("Flow Color", &flowcolor[0]);
			ImGui::ColorEdit3("Edge Color", &edgecolor[0]);

			ImGui::EndTabItem();

		}

		ImGui::EndTabBar();

	}

	};

	//Define the rendering pipeline
	Tiny::view.pipeline = [&](){

		Tiny::view.target(glm::vec3(0));	//Clear Screen to white

		triangleshader.use();
		triangleinstance.render(GL_TRIANGLE_STRIP);

		pointshader.use();
		pointmodel.render(GL_POINTS);

		//Voronoi Graph

		if(drawmesh){
			centermodel.indexed = true;
			centermodel.render(GL_LINES);
			centermodel.indexed = false;
			centermodel.render(GL_POINTS);
		}

		//Render the Particles
		particlemodel.render(GL_POINTS);

	};

	//Execute the render loop
	Tiny::loop([&](){

		if(paused) return;

		//Compute the Flowing Particle Positions

		pt += 0.05f;
		if(pt >= 1.0f) pt = 0.0f;
		initparticles(d);
		for(int i = 0; i < pathlength; i++)
			updateparticles(d);
		particlepos(d);
		particlemodel.construct(pointconstruct, ppos, flowcolor);

		//Execute the Foam Recomputation

		std::vector<glm::vec2> newpoints = points;

		//Iterate over the Triangles...
		for(size_t t0 = 0; t0 < d.triangles.size()/3; t0++){

			for(int e = 0; e < 3; e++){

				int pA = d.triangles[3*t0+e];		//Point A
				int pB = d.halfedges[3*t0+e];  	//Point B (HalfEdge)
				pB = d.triangles[pB];						//Point B

				int t1 = pB/3;									//Other Triangle

				glm::vec2 pdir = points[pB] - points[pA];			//Edge Direction
				glm::vec2 cdir = centers[t1] - centers[t0];  	//Circumcenter Distance

				//Average Catchment
				float c = 1.0f/3.0f*(catchment[3*t0+0] + catchment[3*t0+1] + catchment[3*t0+2]);

				//Equilibrium Distance (i.e. Foam Repulsion)
				if(equilibriation){
					newpoints[pA] += dt*energy*glm::normalize(pdir)*(length(pdir)-scale);
					newpoints[pB] -= dt*energy*glm::normalize(pdir)*(length(pdir)-scale);
				}

				//Contractive Flow
				if(contractiveflow){
					newpoints[pA] += dt*energy*glm::normalize(pdir)*(length(pdir)-scale*(1.0f-c/convergence));
					newpoints[pB] -= dt*energy*glm::normalize(pdir)*(length(pdir)-scale*(1.0f-c/convergence));
				}

				//Expansive Flow
				if(expansiveflow){
					newpoints[pA] += dt*energy*glm::normalize(pdir)*(length(pdir)-scale*c/convergence);
					newpoints[pB] -= dt*energy*glm::normalize(pdir)*(length(pdir)-scale*c/convergence);
				}

			}

		}

		//Update the Triangulation, Update the Rendering Data

		points = newpoints;
		coords.clear();
		for(size_t i = 0; i < points.size(); i++){
			coords.push_back(points[i].x);
			coords.push_back(points[i].y);
		}

		d.compute(coords);
		deriveproperties(d);
		accumulate(d);

		//Recompute Rendering Models
		pointmodel.construct(pointconstruct, points, edgecolor);
		centermodel.construct(triangleconstruct, d);

		//Buffer the Points and Triangle Indices
		triangleshader.buffer("points", points);
		triangleinstance.updateBuffer(triangles, 0);

	});

	Tiny::quit();

	return 0;

}
