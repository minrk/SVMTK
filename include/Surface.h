// Copyright (C) 2018-2020 Lars Magnus Valnes 
//
// This file is part of Surface Volume Meshing Toolkit (SVM-TK).
//
// SVM-Tk is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// SVM-Tk is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SVM-Tk.  If not, see <http://www.gnu.org/licenses/>.
#ifndef Surface_H


#define Surface_H


#ifndef BOOST_PARAMETER_MAX_ARITY
# define BOOST_PARAMETER_MAX_ARITY 12
#endif
 
// Local
#include "Slice.h" 
#include "read_polygons_STL.h"
#include "surface_mesher.h"
#include "reconstruct_surface.h"

// BOOST
#include <boost/foreach.hpp>
#include <boost/multi_array.hpp>
//  STL
#include <CGAL/IO/STL_reader.h>
#include <CGAL/centroid.h>
#include <CGAL/extract_mean_curvature_flow_skeleton.h>
//---------------------------------------------------------
// CGAL- Polygon_mesh_processing 
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/Polygon_mesh_processing/stitch_borders.h>
#include <CGAL/Polygon_mesh_processing/triangulate_hole.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/clip.h>
#include <CGAL/Polygon_mesh_processing/connected_components.h>
#include <CGAL/Polygon_mesh_processing/smooth_shape.h>
#include <CGAL/Polygon_mesh_processing/bbox.h>

#include <CGAL/Polygon_mesh_slicer.h>
// CGAL accelerated queries
#include <CGAL/Search_traits_3.h>
#include <CGAL/Search_traits_adapter.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>
#include <CGAL/squared_distance_3.h> 

//#include <CGAL/Poisson_reconstruction_function.h>

// CGAL Surface mesh simplificiation 
#include <CGAL/Surface_mesh_shortest_path.h>
#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Count_ratio_stop_predicate.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Midpoint_and_length.h>

#include <CGAL/IO/OFF_reader.h>
#include <CGAL/IO/output_surface_facets_to_polyhedron.h>
#include <CGAL/Kernel/global_functions.h>
#include <sys/stat.h>

#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>


#if CGAL_MAJOR_VERSION>4 
std::cout <<"YES" << std::endl;
#endif 


template< typename Mesh> // seperate because -> typedef mesh in Surface
bool load_surface(const std::string file, Mesh& mesh) // new name load polygons if load fails
{
   
  std::ifstream input(file);

  std::string extension = file.substr(file.find_last_of(".")+1);//TODO: FIX boost linking problem
  if (!input)
  {
    std::cerr << "Cannot open file " << std::endl;
    return false;
  }

  typedef typename Mesh::Point Point_3;
  
  
  std::vector<Point_3> points;
  std::vector< std::vector<std::size_t> > polygons;

  if ( extension=="off")
  {
     std::cout<< "reading off" << std::endl;
     if (!CGAL::read_OFF(input, points, polygons))
     {
         std::cerr << "Error parsing the OFF file " << std::endl;
         return false;
     }
     std::cout<< "finished" << std::endl;
  }
  else if ( extension=="stl")
  {
     if (!read_polygons_STL(input, points, polygons)) 
     {
         std::cerr << "Error parsing the STL file " << std::endl;
         return false;
     }
  }
  else
  {
     std::cerr << "Error unkown file" << std::endl;
     return false;
  }

  CGAL::Polygon_mesh_processing::orient_polygon_soup(points, polygons);


  CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(points, polygons,mesh);

  CGAL::Polygon_mesh_processing::orient(mesh) ;


  if (CGAL::is_closed(mesh) && (!CGAL::Polygon_mesh_processing::is_outward_oriented(mesh)))
  {
    std::cout<< "reverse_face_orientation"<< std::endl;

    CGAL::Polygon_mesh_processing::reverse_face_orientations(mesh);
  }
  return true;
}



template< typename Surface>  // FIXME:
void surface_overlapp(Surface& surf1 , Surface& surf2 )
{
    
    typedef typename Surface::vertex_vector vertex_vector;
    typedef typename Surface::vertex_descriptor vertex_descriptor;
   

    std::map< vertex_descriptor, double> map1,map2;

    vertex_vector p1 = surf1.get_vertices();
    vertex_vector p2 = surf2.get_vertices();

    
    p1  = surf2.inside(surf1,p1);
    p2  = surf1.inside(surf2,p2);
    
    int iter =0;
  
    while (  !p2.empty() or  !p1.empty() )
    {
            map1 = surf1.shortest_edge_map(p1,-0.1);
            map2 = surf2.shortest_edge_map(p2,-0.1);

            surf1.adjusting_boundary_region(map1.begin() ,map1.end());
            surf2.adjusting_boundary_region(map2.begin() ,map2.end());

            surf1.smooth_taubin_region(p1.begin(), p1.end(),2);   
            surf2.smooth_taubin_region(p2.begin(), p2.end(),2);    

            p1 = surf2.inside(surf1,p1);
            p2 = surf1.inside(surf2,p2);
            

            if ( iter++>300)
            {
                std::cout << "Failed to converge in 300 steps, terminating" << std::endl;
                break;
            }

    }

    


}


template< typename Surface> 
void surface_overlapp(Surface& surf1 , Surface& surf2 , Surface& domain)
{
    typedef typename Surface::vertex_vector vertex_vector;
    typedef typename Surface::vertex_descriptor vertex_descriptor;

    std::map< vertex_descriptor, double> map1,map2;

    vertex_vector p1 = surf1.get_vertices();
    vertex_vector p2 = surf2.get_vertices();

    p1  = surf2.inside(surf1,p1);
    p2  = surf1.inside(surf2,p2);
    p1  = domain.outside(surf1,p1);
    p2  = domain.outside(surf2,p2);
    int iter =0;
    
    while (  !p2.empty() or  !p1.empty() )
    {
        map1 = surf1.shortest_edge_map(p1,-0.1);
        map2 = surf2.shortest_edge_map(p2,-0.1);

        surf1.adjusting_boundary_region(map1.begin() ,map1.end());
        surf2.adjusting_boundary_region(map2.begin() ,map2.end());
            // After adjusting the boundary smoothing is needed, taubin is least volatile.
        surf1.smooth_taubin_region(p1.begin(), p1.end(),2);   
        surf2.smooth_taubin_region(p2.begin(), p2.end(),2);   

        if ( iter%2==0) 
        {
           p1 = surf2.inside(surf1,p1);
           p2 = surf1.inside(surf2,p2);
        }

        if ( iter++>60)
        {
             break;
        }

    }
    // PART2 

   vertex_vector cp1 = surf1.get_close_points(surf2);
   vertex_vector cp2 = surf2.get_close_points(surf1);


   //seperate_surface as close intersections.
   while (  !cp2.empty() or  !cp1.empty() )
   {

        cp1  = domain.outside(surf1,cp1);
        cp2  = domain.outside(surf2,cp2);

        map1 = surf1.shortest_edge_map(cp1,-0.1); // changed from 0.5
        map2 = surf2.shortest_edge_map(cp2,-0.1);

        surf1.adjusting_boundary_region(map1.begin() ,map1.end());
        surf2.adjusting_boundary_region(map2.begin() ,map2.end());

        surf1.smooth_taubin_region(cp1.begin(), cp1.end(),2);   
        surf2.smooth_taubin_region(cp2.begin(), cp2.end(),2); 

        cp1 = surf1.get_close_points(surf2);
        cp2 = surf2.get_close_points(surf1);
        if ( iter++>60)
            {
                break;
            }
   }


}
template< typename Surface> 
std::shared_ptr<Surface> morphological_surface_union( Surface& surf1 , Surface& surf2 )
{
     // EXPERIMENTAL
     typedef typename Surface::vertex_vector vertex_vector;
     typedef typename Surface::vertex_descriptor vertex_descriptor;

     std::map< vertex_descriptor, double> map1,map2;

     vertex_vector p1 = surf1.get_vertices(); 
     vertex_vector p2 = surf2.get_vertices(); 
   
     p1  = surf2.inside(surf1,p1);
     p2  = surf1.inside(surf2,p2);
     
     surf1.normal_cluster(p1);
     surf2.normal_cluster(p2);

     map1 = surf1.shortest_edge_map(p1,1.0);
     map2 = surf2.shortest_edge_map(p2,1.0);

     surf1.adjusting_boundary_region(map1.begin() ,map1.end());
     surf2.adjusting_boundary_region(map2.begin() ,map2.end());

     surf1.smooth_taubin_region(p1.begin(), p1.end(),2);   
     surf2.smooth_taubin_region(p2.begin(), p2.end(),2); 

     std::shared_ptr<Surface> result(new Surface()); 
     CGAL::Polygon_mesh_processing::corefine_and_compute_union(surf1.get_mesh(), surf2.get_mesh(), result->get_mesh());


    return result;
}



class Surface
{
   public:
    // Essential TODO: CLEAN UP
    typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel; 

    typedef Kernel::Point_3 Point_3;
    typedef Kernel::Plane_3 Plane_3;
    typedef Kernel::FT FT;    
    typedef Kernel::Vector_3 Vector_3;

    typedef CGAL::Surface_mesh<Point_3> Mesh;
    typedef CGAL::Surface_mesh_default_triangulation_3 Tr;
    typedef CGAL::Side_of_triangle_mesh<Mesh,Kernel> Inside; // declear inside implementation ?
    typedef CGAL::Polyhedron_3<Kernel> Polyhedron;   
    typedef CGAL::Vertex_around_target_circulator<Mesh> HV_const_circulator;
    //  Auxilliary
   

    // BOOST is regulary used in CGAL examples 
    typedef boost::graph_traits<Mesh>::edge_descriptor        edge_descriptor;
    typedef boost::graph_traits<Mesh>::halfedge_descriptor    halfedge_descriptor;
    typedef boost::graph_traits<Mesh>::face_descriptor        face_descriptor;
    typedef boost::graph_traits<Mesh>::vertex_descriptor      vertex_descriptor; //  vertex    
    typedef boost::graph_traits<Mesh>::vertices_size_type                    size_type;
    typedef boost::property_map<Mesh,CGAL::vertex_point_t>::type             Vertex_point_pmap; 

    // ???FIXME
    typedef std::vector<Point_3> Polyline;
    typedef std::vector<Polyline> Polylines;
    typedef std::vector<std::size_t>  Face;
    typedef std::vector<Surface::Point_3>                   point_vector;
    typedef std::vector<vertex_descriptor>                  vertex_vector;       
    typedef std::vector<face_descriptor>                    face_vector; 
    typedef Mesh::Vertex_index Index;//??
    

    //typedef std::pair<Point_3, Vector_3> RPwn;
    
    // Point search typdefs
    typedef CGAL::Search_traits_3<Kernel>                                                Traits_base;
    typedef CGAL::Search_traits_adapter<vertex_descriptor,Vertex_point_pmap,Traits_base> Traits;
    typedef CGAL::Orthogonal_k_neighbor_search<Traits>                                   K_neighbor_search;
    typedef K_neighbor_search::Tree                                                      Tree;
    typedef Tree::Splitter                                                               Splitter;
    typedef K_neighbor_search::Distance                                                  Distance;
 
    Surface(){} 
    Surface(Polyhedron &polyhedron); 
    Surface(std::vector<Point_3>& points, std::vector<Face>& faces ); 
    Surface(const std::string  filename1, const std::string  filename2);
    Surface(const std::string  filename);
    ~Surface(){}

    // ----CSG-------------------- overload with Point_3 ?? 
   
    bool is_point_inside(Point_3 point_3);
    //void make_cone( Point_3 p0, Point_3 p1, double r0 , double r1 ,int number_of_segments=360 ) {} 
    void make_cone_(    double x0, double y0, double  z0,  double x1, double y1, double z1, double r0, int number_of_segments=360) ;
    void make_cylinder( double x0, double y0, double  z0,  double x1, double y1, double z1,       double radius,  int number_of_segments=360) ;
    void make_cone(     double x0, double y0, double  z0,  double x1, double y1, double z1, double r0,double r1,  int number_of_segments=360) ; 
    void make_cube(     double x0, double y0, double  z0,  double x1, double y1, double z1, int N=10);
    void make_cube(     double x0, double y0, double  z0,  double x1, double y1, double z1, int Nx , int Ny, int Nz);
    void make_sphere(   double x0, double y0, double  z0,  double r0);
    //-----Boolean operations -----
    void surface_intersection( Surface& other);
    void surface_difference(   Surface& other);
    void surface_union(        Surface& other);    
    // ---- Basic opertaions ------
    Mesh& get_mesh() {return mesh;}
    void clear(){ mesh.clear();}
    int num_faces()    const {return mesh.number_of_faces();}
    int num_edges()    const {return mesh.number_of_edges();}
    int num_vertices() const {return mesh.number_of_vertices();}
    int num_self_intersections();
    void save(const std::string outpath);
   
    // template for the option of different kernels 
    template< typename Polyhedron_3>
    void get_polyhedron(Polyhedron_3 &polyhedron_3 ){CGAL::copy_face_graph(mesh,polyhedron_3);}
             
    int  fill_holes();                       
    bool triangulate_faces();                   // TODO: Proper test  
    void triangulate_hole();  

    void split_edges(double  target_edge_length);
    int  collapse_edges(const double stop_ratio);
    

    //-------------Surface operations -----------
    void isotropic_remeshing(double target_edge_length, unsigned int nb_iter, bool protect_border);
    void adjust_boundary(const double c);
    void smooth_laplacian(const double c, int iter);
    void smooth_taubin(const size_t nb_iter); 
    void seperate_narrow_gaps();
    void clip(double x1,double x2, double x3 ,double x4, bool clip);
    std::map<vertex_descriptor,double> seperate_close_surfaces(Surface& other);

    // std::shared_ptr<Slice> mesh_slice( Point_3 p, Vector_3 n) ;
    std::shared_ptr<Slice> mesh_slice(double x1,double x2, double x3 ,double x4) ;
    std::shared_ptr<Slice> mesh_slice(Plane_3 plane) ;
    //  TODO:rename to 
    std::pair<double,double> span(int direction);
    // TODO: rename, prupose cluster vertices with same normal as input vertices  
    void normal_cluster( vertex_vector &input);
    
    void smooth_shape(double time,int nb_iterations);

    void adjusting_boundary_region(std::map<vertex_descriptor,double>::iterator begin, std::map<vertex_descriptor,double>::iterator end );

    // could avoid template
    template<typename InputIterator>   
    void adjusting_boundary_region(InputIterator begin , InputIterator  end, const double c);
    template<typename InputIterator>
    void smooth_laplacian_region(InputIterator begin , InputIterator end   ,const double c);
    template<typename InputIterator>
    void smooth_taubin_region(InputIterator begin , InputIterator end      ,const size_t iter);

    void write_STL(const std::string filename);
    std::map< vertex_descriptor, double> shortest_edge_map(Surface::vertex_vector &vector,double adjustment =-0.5) ;
   
    //--------Inquries --------------
    vertex_vector inside(Surface  &other, Surface::vertex_vector &vertices);
    vertex_vector outside(Surface &other, Surface::vertex_vector &vertices);


    vertex_vector get_vertices();
    point_vector  get_points(); 
    vertex_vector get_close_points(Surface &other);

    Polyline  shortest_surface_path(Point_3 source, Point_3 target);
    // -------Point operations -----
    void reconstruct( double sm_angle = 20.0,
                          double sm_radius = 100.0,
                          double sm_distance = 0.25,
                          double approximation_ratio = 0.02,
                          double average_spacing_ratio = 5.0);


    Polylines mean_curvature_flow();
   



    void strictly_inside(Surface& other);

    template<typename Implicit_function>  
    void implicit_surface(Implicit_function implicit_function,
             double bounding_sphere_radius,
             double angular_bound=30.,
             double radius_bound=0.1 ,
             double distance_bound=0.1   );

    void cylindric_extension(Point_3& p1,double& radius, double length, bool normal=true ); 
    vertex_vector closest_points(Point_3 p1, int num = 8);
   protected:
    Mesh mesh;



};


////////////////////////////////////////////////////////////////////////////////////////////
void Slice::add_surface_domains(std::vector<Surface> surfaces)
{
   if ( this->cdt.number_of_faces()==0)
   {
      std::cout<<"create mesh first"<< std::endl; 
      return;
   }
   DefaultMap map =DefaultMap();
   add_surface_domains(surfaces,map);

}


void Slice::add_surface_domains(std::vector<Surface> surfaces, AbstractMap& map) //TODO:RENAME
{
   if ( this->cdt.number_of_faces()==0)
   {
      std::cout<<"create mesh first"<< std::endl; 
      return;
   }
   typedef boost::dynamic_bitset<>   Bmask;
   typedef std::pair<int,int>  Pid; // patch id 
   typedef std::map<Pid,int> Pid_map;
   int fn,fi;
   
   Pid_map pid_map_;
   Pid spp;
   std::map<Face_handle,Bmask> masks;
   int index_counter=1;
   for ( auto surf :  surfaces) 
   {
       Surface::Inside inside(surf.get_mesh());
       for(CDT::Face_iterator fit = cdt.faces_begin(); fit != cdt.faces_end(); ++fit)
       {
          Point_2 p2 =  CGAL::centroid(fit->vertex(0)->point() ,fit->vertex(1)->point(),fit->vertex(2)->point());  
          Point_3 p3 =  this->plane.to_3d(p2);
          CGAL::Bounded_side res = inside(p3);
          if (res == CGAL::ON_BOUNDED_SIDE)
             masks[fit].push_back(1);
          else
             masks[fit].push_back(0);
       }

   } 
   for (auto bit : masks)
   {
       bit.first->info()=map.index(bit.second);
       if (bit.first->info()==0)
          cdt.delete_face(bit.first);
   
   }
   Edge ei;
   for(Face_iterator fit = cdt.finite_faces_begin(); fit != cdt.finite_faces_end(); ++fit) 
   {
      fi = fit->info(); 
      for( int i =0 ;i<3;++i)
      {
         Edge ei(fit,i); 
         if (fit->neighbor(i)->is_in_domain())
            fn= fit->neighbor(i)->info();
         else 
            fn=0;             
            if (fn!=fi)
            {     
               if (fn>fi)
                  spp={fn,fi};
               else
                  spp={fi,fn};
               std ::pair<Pid_map::iterator, bool> is_insert_successful = pid_map_.insert(std::make_pair(spp,index_counter));
               if(is_insert_successful.second)
               { 
                 index_counter++;
                 std::cout <<  spp.first << "\t" << spp.second << index_counter<< std::endl;
               }
               this->edges[ei] = pid_map_[spp];    
            }
            else
            {
               this->edges[ei] =  0;
            }          
         }
     }
        
}

void Slice::slice_surfaces(std::vector<Surface> surfaces) // TODO : DECIDE return slice or member function 
{
   
   for ( auto surf : surfaces ) 
   {
         std::shared_ptr<Slice> temp = surf.mesh_slice(this->plane);     
         this->add_constraints(*temp.get()); 
   }


}  
std::shared_ptr<Surface> Slice::as_surface() 
{
  // TODO: why ?? FIXME
  typedef std::vector<std::size_t> Face ;
  typedef CDT::Vertex_handle Vertex_handle;

  std::vector<Point_3> points ;
  std::vector<Face> faces;
  std::map<Vertex_handle, int> index_of_vertex;
  int i = 0;
  for(CDT::Point_iterator it = cdt.points_begin() ;  it != cdt.points_end(); ++it, ++i)
  {
       points.push_back(plane.to_3d(*it));  
       index_of_vertex[it.base()] = i;
  }
  for(CDT::Face_iterator fit = cdt.faces_begin(); fit != cdt.faces_end(); ++fit)
  {
       Face temp;
       temp.push_back( index_of_vertex[fit->vertex(0)] ) ;
       temp.push_back( index_of_vertex[fit->vertex(1)] ) ;
       temp.push_back( index_of_vertex[fit->vertex(2)] ) ;
       faces.push_back(temp);
  }
   std::shared_ptr<Surface> surf(new  Surface(points, faces)) ; // FIXME: use make_shared instead
   return surf;  
}
///////////////////////////////////////////////////////////////////////////////////////////



void Surface::strictly_inside(Surface& other)
{  
     // white.(pial)
     //  explanation :
     // Find all vertices of surface (this->mesh) that are outside Surface(other) and
     // move them so that all vertices are inside Surface (other)

     std::map< vertex_descriptor, double> map1; 
     vertex_vector points = this->get_vertices(); // white pointa
   
     points  = other.outside(*this,points); // pial(white,points)

     int iter=0;
     while (  !points.empty() )
     {
            map1 = this->shortest_edge_map(points,-0.1); 
            this->adjusting_boundary_region(map1.begin() ,map1.end());
            //this->smooth_taubin_region(points.begin(), points.end(),2);   
            points = other.outside(*this,points);

            if (iter++>=300)
            {
              break;
            }

     }
     
     
}
Surface::vertex_vector Surface::closest_points(Point_3 p1, int num)
{
   Vertex_point_pmap vppmap = get(CGAL::vertex_point,mesh);
   vertex_vector results;

   Tree tree(vertices(mesh).begin(),
            vertices(mesh).end(),
            Splitter(),
            Traits(vppmap)
   );

   Distance tr_dist(vppmap);

   FT distance, edgeL;
   bool flag;
   
   K_neighbor_search search(tree,p1, num,0,true,tr_dist); //tree.closest_point(point_query); must be second closest
 
   for ( auto vit=search.begin() ; vit!=search.end(); ++ vit) 
   {
        results.push_back(vit->first) ; // The closest point is the query point, therefore choose second point as closest. 
   }
   return results;
} 

// double x0, double y0, double  z0,  double x1, double y1, double z1, double radius,  int number_of_segments) 
void Surface::cylindric_extension(Point_3& p1, double& radius, double length, bool normal)
{
   // TODO: Algorithm  
   //       2) Handle concave surface 
   //       3) adjust boundary in direction -> translation of points 

   Point_3 p3, p4;
   FT distance, l(length); 
   Vector_3 n;

   Surface::Inside inside_poly2(mesh);
   CGAL::Bounded_side res = inside_poly2(p1);
   assert( res == CGAL::ON_UNBOUNDED_SIDE );

   vertex_vector vertices = closest_points(p1,1);
   vertices = closest_points(mesh.point(vertices[0]),8); 

   std::vector<Point_3> points;
   for (auto i= vertices.begin(); i!=vertices.end();++i)
   {
   }
   // TODO: Handle vertices vector to point vector
   Point_3 p2 = CGAL::centroid(points.begin(),points.end(),CGAL::Dimension_tag<0>()); 
 
  
   if (!normal) 
   {
        n = p1-p2; 
        distance = CGAL::sqrt(n.squared_length());
        n=n/distance;  
   }
   else 
   {    
        for (auto i= vertices.begin(); i!=vertices.end();++i)
        {
            n=n+ CGAL::Polygon_mesh_processing::compute_vertex_normal(*i,mesh);
        }
        distance = CGAL::sqrt(n.squared_length());
        n=n/distance;  
   }


   p3 = p2 + n*length;  
   p4 = p2 -n*radius;

 
   double x0  = CGAL::to_double(p4.x());
   double y0  = CGAL::to_double(p4.y());
   double z0  = CGAL::to_double(p4.z());
   double x1  = CGAL::to_double(p3.x());
   double y1  = CGAL::to_double(p3.y());
   double z1  = CGAL::to_double(p3.z());

   Surface cylinder;
   cylinder.make_cylinder(x0,y0,z0,x1,y1,z1,radius,60); // NOTE

   vertex_vector vec1  = cylinder.closest_points(p3,1);

   cylinder.normal_cluster(vec1);

   this->surface_union(cylinder); 

}


std::map<Surface::vertex_descriptor, double> Surface::shortest_edge_map(Surface::vertex_vector &vector, double adjustment )
{
   
   std::map< vertex_descriptor, double> results;
   for (vertex_descriptor v_it : vector)
   {
        Point_3 current = mesh.point(v_it);
        HV_const_circulator vbegin(mesh.halfedge(v_it),mesh), done(vbegin);

        FT min_edge = FT(100);
        do
        {
          FT temp = CGAL::squared_distance(current, mesh.point(*vbegin) ); 
          if (temp < min_edge )
          {
             min_edge = temp;
          }
       
         *vbegin++;
        }while(vbegin!=done);

   results[v_it] = adjustment*static_cast<double>(CGAL::sqrt(min_edge));
   }
   return results;
        
}

std::pair<double,double> Surface::span(int direction)
{
     auto bbox_3 = CGAL::Polygon_mesh_processing::bbox(mesh);
     std::pair<double,double> span (bbox_3.min(direction),bbox_3.max(direction) );
     return span;
     
}

void Surface::write_STL(const std::string filename)
{
    std::ofstream file(filename);
    file.precision(6);
    Vector_3 n;

    file << "solid "<< filename << std::endl;

    for (auto f : mesh.faces() )
    {

       n = CGAL::Polygon_mesh_processing::compute_face_normal(f,mesh);


       file << "facet normal " << n.x() << " " << n.y()   << " " << n.z() <<std::endl;
       file << "outer loop"<< std::endl;
       CGAL::Vertex_around_face_iterator<Mesh> vbegin, vend; //?????
       for(boost::tie(vbegin, vend) = vertices_around_face(mesh.halfedge(f), mesh); vbegin != vend; ++vbegin)
       {
          file << "\t" << "vertex " << mesh.point(*vbegin).x() <<" "<< mesh.point(*vbegin).y() <<" "<< mesh.point(*vbegin).z()  << std::endl;  
            
       }
       file <<"endloop" << std::endl;
       file <<"endfacet"<< std::endl;
    }
    file << "endsolid"  << std::endl;

}

template<typename Implicit_function>
void Surface::implicit_surface(Implicit_function implicit_function,
             double bounding_sphere_radius,
             double angular_bound,
             double radius_bound,
             double distance_bound)
{
     surface_mesher(mesh,implicit_function,bounding_sphere_radius,angular_bound,radius_bound, distance_bound);
}


template<typename InputIterator >
void Surface::adjusting_boundary_region(InputIterator begin , InputIterator  end, const double c)
{
  
  std::vector<std::pair<vertex_descriptor, Point_3> > smoothed; 
  for ( ; begin != end; ++begin)
  {
      Vector_3 delta= CGAL::Polygon_mesh_processing::compute_vertex_normal(*begin,mesh); 
      Point_3 p = mesh.point(*begin) + c*delta;
      smoothed.push_back(std::make_pair(*begin, p));
  }

  for (std::pair<vertex_descriptor, Point_3> s : smoothed)
  {
      mesh.point(s.first) = s.second;
  }


}

void Surface::adjusting_boundary_region(std::map<vertex_descriptor,double>::iterator begin, std::map<vertex_descriptor,double>::iterator end) // map or two vectors
{

  std::vector<std::pair<vertex_descriptor, Point_3> > smoothed; 

  for ( ; begin != end; ++begin)
  {
      Vector_3 delta= CGAL::Polygon_mesh_processing::compute_vertex_normal(begin->first,mesh); 
      Point_3 p = mesh.point(begin->first) + begin->second*delta;
      smoothed.push_back(std::make_pair(begin->first, p));
  }
  for (std::pair<vertex_descriptor, Point_3> s : smoothed)
  {
      mesh.point(s.first) = s.second;
  }
}





template<typename InputIterator >
void Surface::smooth_laplacian_region(InputIterator begin , InputIterator end ,const double c)
{
 
  std::vector<std::pair<vertex_descriptor, Point_3> > smoothed;
  for ( ; begin != end; ++begin)
  {

      Point_3 current = mesh.point(*begin);
      Vector_3 delta=CGAL::NULL_VECTOR;
      HV_const_circulator vbegin(mesh.halfedge(*begin),mesh), done(vbegin);
      do
      {
          delta += Vector_3(mesh.point(*vbegin) - current);
          *vbegin++;
      }while(vbegin!=done);

      Point_3 p = current + c*delta/mesh.degree(*begin);  // WHY C
      smoothed.push_back(std::make_pair(*begin, p));
  }
  for (std::pair<vertex_descriptor, Point_3> s : smoothed)
  {
      mesh.point(s.first) = s.second;
  }
}







//--------------------------------------------
//      CGALSURFACE FUNCTIONS
//--------------------------------------------
Surface::Surface(Polyhedron &polyhedron) {
    CGAL::copy_face_graph(polyhedron, mesh);
}

Surface::Surface(const std::string  filename1, const std::string  filename2)
{   
    Mesh mesh1,mesh2; 
    load_surface(filename1,mesh1);
    load_surface(filename2,mesh2);
    CGAL::Polygon_mesh_processing::corefine_and_compute_union(mesh1, mesh2,mesh);
}

Surface::Surface(const std::string filename)
{
    load_surface(filename,mesh);
}

Surface::Surface(std::vector<Point_3>& points, std::vector<Face>& faces)
{
  CGAL::Polygon_mesh_processing::orient_polygon_soup(points, faces);
  CGAL::Polygon_mesh_processing::polygon_soup_to_polygon_mesh(points, faces,mesh);

  if (CGAL::is_closed(mesh) && (!CGAL::Polygon_mesh_processing::is_outward_oriented(mesh)))
  {
    std::cout<< "reverse_face_orientation"<< std::endl;
    CGAL::Polygon_mesh_processing::reverse_face_orientations(mesh);
  }

}
// TODO: Boolean operation to overloaded operators ?
void Surface::surface_intersection( Surface& other)
{
     CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(mesh, other.get_mesh(),mesh) ;
}
void Surface::surface_difference( Surface& other)
{
     Mesh out_mesh;
     CGAL::Polygon_mesh_processing::corefine_and_compute_difference(mesh , other.get_mesh(),mesh);
 
}

void Surface::surface_union(Surface& other)
{
     CGAL::Polygon_mesh_processing::corefine_and_compute_union(mesh, other.get_mesh(), mesh);
}




/* 
     object: take into account corpus collusom 
*/
void Surface::normal_cluster(Surface::vertex_vector &input)
{
  int size = input.size();

  for (int i = 0 ; i < size ; ++i)
  {
      Vector_3 n1 = CGAL::Polygon_mesh_processing::compute_vertex_normal(input[i],mesh); 

      HV_const_circulator vbegin(mesh.halfedge(input[i]),mesh), done(vbegin);
      do
      {
          Vector_3 n2= CGAL::Polygon_mesh_processing::compute_vertex_normal(*vbegin,mesh);  
           
          if ( n1*n2 > CGAL::sqrt(n1.squared_length()) * CGAL::sqrt(n2.squared_length())*0.80 )// TODO: include as argument
          {
              vertex_descriptor tar =   *vbegin;
            
              if(std::find(input.begin(), input.end(), tar) == input.end()) 
              {
                 input.push_back(tar ); 
                 ++size;
              } 
          } 

          *vbegin++;
      }while(vbegin!=done);
  }


}




void Surface::smooth_taubin(const size_t nb_iter) {
    for (size_t i = 0; i < nb_iter; ++i) {
        this->smooth_laplacian(0.8,1);
        this->smooth_laplacian(-0.805,1);
    }
}
template<typename InputIterator >
void Surface::smooth_taubin_region(InputIterator begin , InputIterator end ,const size_t nb_iter)
{
    for (size_t i = 0; i < nb_iter; ++i) {
        this->smooth_laplacian_region(begin,end,0.8);
        this->smooth_laplacian_region(begin,end,-0.805);
    }
}

int Surface::num_self_intersections() {
    std::vector< std::pair<face_descriptor, face_descriptor> > intersected_tris;
    CGAL::Polygon_mesh_processing::self_intersections(mesh, std::back_inserter(intersected_tris));
    return intersected_tris.size();  // Could actually return the triangles themselves -> return facets
}

int Surface::collapse_edges(const double stop_ratio) {
    
    CGAL::Surface_mesh_simplification::Count_ratio_stop_predicate<Mesh> stop(stop_ratio);

    const int r = CGAL::Surface_mesh_simplification::edge_collapse(
        mesh,
        stop,
        CGAL::parameters::get_cost(CGAL::Surface_mesh_simplification::Edge_length_cost<Mesh>())
            .get_placement(CGAL::Surface_mesh_simplification::Midpoint_placement<Mesh>()));

    return r;
}
std::shared_ptr<Slice> Surface::mesh_slice(double x1,double x2, double x3 ,double x4)
{
   assert( (x1!=0) or (x2!=0) or (x3!=0) ) ;
   Plane_3 plane = Plane_3(x1, x2, x3, x4);
   return this->mesh_slice(plane);

}

std::shared_ptr<Slice> Surface::mesh_slice(Plane_3 plane_3)  // const ?? 
{
     // 2D 
     typedef Kernel::Point_2 Point_2;
     typedef std::vector<Point_2> Polyline_2;
     typedef std::vector<Point_3>  Polyline_3; 
     typedef std::vector<Polyline_2> Polylines_2;
     typedef std::vector<Polyline_3> Polylines; 
     //-------------------------------------------------------------
     // Intersection Polylines
     //-------------------------------------------------------------
     CGAL::Polygon_mesh_slicer<Mesh, Kernel> slicer(mesh); 
     Polylines polylines_3D;
     slicer(plane_3, std::back_inserter(polylines_3D));
     //-------------------------------------------------------------
     // Polylines to 2D 
     //-------------------------------------------------------------
     std::vector<std::vector<Point_2>> polylines_2;
     for ( auto  pol = polylines_3D.begin(); pol != polylines_3D.end(); ++pol ) 
     {
         std::vector<Point_2> result;
         std::vector<Point_2> polyline_2;
         for ( auto pit = pol->begin(); pit != pol->end(); ++pit)
         {
               polyline_2.push_back(plane_3.to_2d(*pit));
         }
         polylines_2.push_back(polyline_2);
     }


     std::shared_ptr<Slice> slice(new Slice(plane_3,polylines_2)); 
     //slice->add_constraints(polylines_2);
     //slice->set_plane(plane_3); // export to 3D

     return slice;
}     





void Surface::isotropic_remeshing(double target_edge_length, unsigned int nb_iter, bool protect_border)
{

     CGAL::Polygon_mesh_processing::split_long_edges(edges(mesh), target_edge_length,mesh);
     CGAL::Polygon_mesh_processing::isotropic_remeshing(faces(mesh),
                              target_edge_length,
                              mesh,
                              CGAL::Polygon_mesh_processing::parameters::number_of_iterations(nb_iter)
                              .protect_constraints(protect_border));

}

void Surface::clip(double x1,double x2, double x3 ,double x4, bool clip){
   CGAL::Polygon_mesh_processing::clip(mesh, Plane_3(x1,x2,x3,x4), CGAL::Polygon_mesh_processing::parameters::clip_volume(clip));

}
//-----------------  TODO: PERFORM TESTS----------------------------------------------------
void Surface::triangulate_hole()
{   
     BOOST_FOREACH(halfedge_descriptor h, halfedges(mesh))
     {
        std::vector<face_descriptor>  patch_facets;
        if(is_border(h,mesh))
        {
           CGAL::Polygon_mesh_processing::triangulate_hole(mesh, h,back_inserter(patch_facets));
        }
     }
}

int Surface::fill_holes()
{
    unsigned int nb_holes = 0;
    BOOST_FOREACH(halfedge_descriptor h, halfedges(mesh))
    {
      if(is_border(h,mesh))
      {
        std::vector<face_descriptor>  patch_facets;
        std::vector<vertex_descriptor> patch_vertices;
        bool success = CGAL::cpp11::get<0>(
          CGAL::Polygon_mesh_processing::triangulate_refine_and_fair_hole(
                    mesh,
                    h,
                    std::back_inserter(patch_facets),
                    std::back_inserter(patch_vertices),
          CGAL::Polygon_mesh_processing::parameters::vertex_point_map(get(CGAL::vertex_point, mesh)).
                    geom_traits(Kernel())) );

        std::cout << "* Number of facets in constructed patch: " << patch_facets.size() << std::endl;
        std::cout << "  Number of vertices in constructed patch: " << patch_vertices.size() << std::endl;
        std::cout << "  Is fairing successful: " << success << std::endl;
        nb_holes++;
      }
    }

    std::cout << std::endl;
    std::cout << nb_holes << " holes have been filled" << std::endl;

    return nb_holes;
}
bool Surface::triangulate_faces()
{
    CGAL::Polygon_mesh_processing::triangulate_faces(mesh);

    BOOST_FOREACH(face_descriptor fit, faces(mesh))
      if (next(next(halfedge(fit, mesh), mesh), mesh)
          !=   prev(halfedge(fit, mesh), mesh))
        std::cerr << "Error: non-triangular face left in mesh." << std::endl;
    return true;
}

std::vector<Surface::Point_3>  Surface::get_points() 
{
   point_vector result;
   //BOOST_FOREACH(vertex_descriptor v_it, vertices(mesh)
   for ( vertex_descriptor v_it : mesh.vertices())
   {
        result.push_back(mesh.point(v_it) );
   }
   return result;


} 

Surface::vertex_vector Surface::get_vertices()
{
   vertex_vector result;
   //BOOST_FOREACH(vertex_descriptor v_it, vertices(mesh)
   for ( vertex_descriptor v_it : mesh.vertices())
   {
        result.push_back(v_it);
   }
   return result;
}
bool Surface::is_point_inside(Point_3 point_3) 
{
     Surface::Inside inside_poly2(mesh);
     CGAL::Bounded_side res = inside_poly2(point_3);
     if (res == CGAL::ON_BOUNDED_SIDE or res == CGAL::ON_BOUNDARY)
         return true;
     else 
         return false;

}
Surface::vertex_vector Surface::inside(Surface &other, Surface::vertex_vector &vertices)
{
   vertex_vector result;
   Surface::Inside inside_poly2(mesh); // remove? 

   for ( vertex_descriptor v_it : vertices )
   {
      CGAL::Bounded_side res = inside_poly2(other.get_mesh().point(v_it));
      if (res == CGAL::ON_BOUNDED_SIDE or res == CGAL::ON_BOUNDARY)
      {  
         result.push_back(v_it);
      }
   }
  
   return result;
}

Surface::vertex_vector Surface::outside(Surface &other,Surface::vertex_vector &vertices)
{
   // TODO: BETTER STRUCTURE
   // delete instead
   vertex_vector result;
   Surface::Inside inside_poly2(mesh); // remove ??
   for ( vertex_descriptor v_it : vertices )
   {
      CGAL::Bounded_side res = inside_poly2(other.get_mesh().point(v_it));
      if (res == CGAL::ON_UNBOUNDED_SIDE){result.push_back(v_it);}
   }
  
   return result;
}





void Surface::adjust_boundary(const double c)
{
    Mesh::Vertex_range::iterator  vb = mesh.vertices().begin(), ve=mesh.vertices().end();
    Surface::adjusting_boundary_region(vb, ve,c);

}
void Surface::smooth_laplacian(const double c, int iter)
{
    Mesh::Vertex_range::iterator  vb = mesh.vertices().begin(), ve=mesh.vertices().end();
    for ( int i = 0 ; i< iter ; ++i)
    {
        Surface::smooth_laplacian_region(vb, ve,c);
    }
}
void Surface::smooth_shape(double time,int nb_iterations)
{
CGAL::Polygon_mesh_processing::smooth_shape(mesh, time, CGAL::Polygon_mesh_processing::parameters::number_of_iterations(nb_iterations));
}

Surface::Polylines Surface::mean_curvature_flow() 
{
    typedef CGAL::Mean_curvature_flow_skeletonization<Surface::Mesh> Skeletonization;
    typedef Skeletonization::Skeleton                             Skeleton;
    typedef Skeleton::vertex_descriptor                           Skeleton_vertex;
    typedef Skeleton::edge_descriptor                             Skeleton_edge;

    struct Construct_polylines
    {   
       const Skeleton& skeleton;
       std::vector<Surface::Point_3> polyline_3;
       std::vector<std::vector<Surface::Point_3>> polylines_3;
 
       Construct_polylines(const Skeleton& skeleton): skeleton(skeleton){}
       void start_new_polyline()
       {
           polyline_3.clear() ;
       } 
       void add_node(Skeleton_vertex v)
       {
           polyline_3.push_back(skeleton[v].point);
       }
       void end_polyline()
       {
          polylines_3.push_back(polyline_3);                     

       }
       std::vector<std::vector<Point_3>>& get_polylines()
       {
           return polylines_3;
       }         
    };

  
    std::vector<std::vector<Surface::Point_3>> polylines_3;
    Skeleton skeleton;
    CGAL::extract_mean_curvature_flow_skeleton(mesh, skeleton);
    Construct_polylines Visitor(skeleton);
    CGAL::split_graph_into_polylines(skeleton,Visitor);
    return Visitor.get_polylines();

}

// TODO : test 
Surface::Polyline Surface::shortest_surface_path(Point_3 source, Point_3 target) 
{ 
      std::vector<Surface::Point_3> points;
      // TODO : combine with nearest point projection onto surface
      typedef CGAL::Surface_mesh_shortest_path_traits<Kernel, Mesh> Traits;
      typedef CGAL::Surface_mesh_shortest_path<Traits> Surface_mesh_shortest_path;
      typedef Surface_mesh_shortest_path::Face_location Face_location;
      typedef CGAL::AABB_face_graph_triangle_primitive<Mesh> Primitive;
      typedef CGAL::AABB_traits<Kernel, Primitive> AABB_Traits;
      typedef CGAL::AABB_tree<AABB_Traits> Tree;

     
      Tree tree(faces(mesh).first, faces(mesh).second, mesh); 
      Surface_mesh_shortest_path shortest_paths(mesh);
      

      Face_location source_location = shortest_paths.locate(source,tree);
      Face_location target_location = shortest_paths.locate(target,tree);


      shortest_paths.add_source_point(source_location);


      shortest_paths.shortest_path_points_to_source_points(target_location.first,target_location.second , std::back_inserter(points));


      return points ;
} 
void Surface::make_cube( double x0, double y0, double  z0,  double x1, double y1, double z1, int N ){
              make_cube( x0,y0,z0,x1,y1,z1, N , N , N );
}

void Surface::make_cube( double x0, double y0, double  z0,  double x1, double y1, double z1, int Nx , int Ny , int Nz) // TODO :allow tilted cubes  vector  
{
     //  ADD mesh resolution error 
     assert(x0!=x1);
     assert(y0!=y1);
     assert(z0!=z1);


     double dx =(x1-x0);  
     double dy =(y1-y0);
     double dz =(z1-z0);
 
     typedef boost::multi_array<int, 3> array_type;
     array_type map(boost::extents[Nx+1][Ny+1][Nz+1]);

     
     //int map[Nx+1][Ny+1][Nz+1]; 
     int index =0;
     for ( int i = 0 ; i< Nx+1 ; ++i)
     {
         for (int j=0 ; j < Ny+1 ; ++j )
         { 
              for (int k=0 ; k < Nz+1 ; ++k )
              {
                  mesh.add_vertex(
                       Point_3(x0+ static_cast<double>(i)*dx/static_cast<double>(Nx),
                               y0+ static_cast<double>(j)*dy/static_cast<double>(Ny),
                               z0+ static_cast<double>(k)*dz/static_cast<double>(Nz) ) ) ;
                     map[i][j][k] = index++;         
              }
         }    
     }


     

     face_vector s1;
     face_vector s2;
     // x0 and x1 
     for (int j=0 ; j < Ny ; ++j )
     { 
         for (int k=0 ; k < Nz ; ++k )
         {
             s1.push_back(mesh.add_face(Index(map[0][j][k])  , Index(map[0][j+1][k+1]),Index(map[0][j+1][k])   ));
             s1.push_back(mesh.add_face(Index(map[0][j][k]), Index(map[0][j][k+1]) , Index(map[0][j+1][k+1])      ));
             s2.push_back(mesh.add_face(Index(map[Nx][j][k]) , Index(map[Nx][j+1][k]) , Index(map[Nx][j][k+1])  ));
             s2.push_back(mesh.add_face(Index(map[Nx][j+1][k]), Index(map[Nx][j+1][k+1]) , Index(map[Nx][j][k+1])   ));
         }
     }

     face_vector s3;
     face_vector s4;
     // y0 and y1 
     for (int i=0 ; i < Nx ; ++i )
     { 
         for (int k=0 ; k < Nz ; ++k )
         {
              s3.push_back(mesh.add_face(Index(map[i][0][k]) , Index(map[i+1][0][k])  ,Index( map[i][0][k+1])      ));
              s3.push_back(mesh.add_face(Index(map[i+1][0][k]),Index( map[i+1][0][k+1])  , Index(map[i][0][k+1])       ));
              s4.push_back(mesh.add_face(Index(map[i][Ny][k])  , Index(map[i][Ny][k+1]) , Index(map[i+1][Ny][k+1])   ));
              s4.push_back(mesh.add_face(Index(map[i+1][Ny][k]), Index(map[i][Ny][k]) , Index(map[i+1][Ny][k+1]) ));
         } 
     }

     face_vector s5;
     face_vector s6;
     // z0 and z1 
     for (int i=0 ; i < Nx ; ++i )
     { 
         for (int j=0 ; j < Ny ; ++j )
         {
              s5.push_back(mesh.add_face(Index(map[i][j][0] ), Index(map[i][j+1][0]) , Index(map[i+1][j+1][0]   ) ));
              s5.push_back(mesh.add_face(Index(map[i][j][0] ), Index(map[i+1][j+1][0])  , Index(map[i+1][j][0] )  ));
              s6.push_back(mesh.add_face(Index(map[i][j][Nz]), Index(map[i+1][j][Nz]), Index(map[i+1][j+1][Nz]  ) ));
              s6.push_back(mesh.add_face(Index(map[i][j][Nz]), Index(map[i+1][j+1][Nz]), Index(map[i][j+1][Nz])   ));
         } 
     }
 
     return;

}
void Surface::make_cylinder( double x0, double y0, double  z0,  double x1, double y1, double z1, double radius,  int number_of_segments) {  
     Surface::make_cone( x0, y0,z0, x1,y1, z1, radius , radius,  number_of_segments ) ;



}
// Handles the case (r0 or r1)== 0
void Surface::make_cone_( double x0, double y0, double  z0,  double x1, double y1, double z1, double r0, int number_of_segments) 
{

     face_vector fv1,fv3;

     Index v0 = mesh.add_vertex(Point_3(x0,y0,z0));
     Index v1 = mesh.add_vertex(Point_3(x1,y1,z1));
        
     Index vb, vt;
  
     double n0 = x1-x0, n1 = y1-y0, n2 = z1-z0;

     double l1 = std::sqrt(n0*n0+n1*n1+n2*n2);

     Vector_3 normal( n0/l1 , n1/l1, n2/l1 );

     double l2 = std::sqrt( (n1-n2)*(n1-n2) + (n2-n0)*(n2-n0) + ( n0 -n1)*(n0-n1) );

     Vector_3 t1( (n1-n2)/l2 , (n2-n0)/l2, (n0-n1)/l2 ); 

     Vector_3 t2 = CGAL::cross_product(normal,t1); // normalized ?? 

     double c  = 360.0/(double)number_of_segments;
     

     for(int i = 0; i < number_of_segments; ++i) 
     {
              
         Point_3 pb= Point_3(x0,y0,z0)  + t1*r0*std::cos(c*i*CGAL_PI/180) + t2*r0*std::sin(c*i*CGAL_PI/180); 
         vb = mesh.add_vertex(pb);

         if ( i!=0)
         {  
             fv1.push_back( mesh.add_face(v0,vb,Index(vb-1)) );
             fv3.push_back( mesh.add_face(Index(vb-1),vb,v1) );
         }

    }
    fv1.push_back(mesh.add_face(Index(2),vb,v0 ) );
    fv3.push_back(mesh.add_face(vb,Index(2),v1) );

   
    double edge_length = r0/10.0; 

    CGAL::Polygon_mesh_processing::isotropic_remeshing(fv1,
                              edge_length,
                              mesh,
                              CGAL::Polygon_mesh_processing::parameters::number_of_iterations(3)
                              .protect_constraints(true));

    CGAL::Polygon_mesh_processing::isotropic_remeshing(fv3,
                              edge_length,
                              mesh,
                              CGAL::Polygon_mesh_processing::parameters::number_of_iterations(3)
                              .protect_constraints(true));


}


void Surface::make_cone( double x0, double y0, double  z0,  double x1, double y1, double z1, double r0, double r1,   int number_of_segments) 
{
     assert((r0*r0+r1*r1)!=0);
     if ( r0==0.0 )
     {
        make_cone_(x1,y1,z1,x0,y0,z0,r1,number_of_segments);
        return;
     }
     else if ( r1==0.0 )
     {
        make_cone_(x0,y0,z0,x1,y1,z1,r0,number_of_segments);
        return;
     }
     face_vector fv1,fv2,fv3;

     Index v0 = mesh.add_vertex(Point_3(x0,y0,z0));
     Index v1 = mesh.add_vertex(Point_3(x1,y1,z1));
        
     Index vb, vt;
  

     double n0 = x1-x0, n1 = y1-y0, n2 = z1-z0;

     double l1 = std::sqrt(n0*n0+n1*n1+n2*n2);

     Vector_3 normal( n0/l1 , n1/l1, n2/l1 );

     double l2 = std::sqrt( (n1-n2)*(n1-n2) + (n2-n0)*(n2-n0) + ( n0 -n1)*(n0-n1) );

     Vector_3 t1( (n1-n2)/l2 , (n2-n0)/l2, (n0-n1)/l2 ); 

     Vector_3 t2 = CGAL::cross_product(normal,t1); // normalized ?? 

     double c  = 360.0/(double)number_of_segments;
     

     for(int i = 0; i < number_of_segments; ++i) 
     {
              
         Point_3 pb= Point_3(x0,y0,z0)  + t1*r0*std::cos(c*i*CGAL_PI/180) + t2*r0*std::sin(c*i*CGAL_PI/180); 
         Point_3 pt = Point_3(x1,y1,z1)  + t1*r1*std::cos(c*i*CGAL_PI/180) + t2*r1*std::sin(c*i*CGAL_PI/180); 

         vb = mesh.add_vertex(pb);
         vt = mesh.add_vertex(pt);

         if ( i!=0)
         {  
             fv1.push_back( mesh.add_face(v0, vb,Index(vb-2)) );
             fv2.push_back( mesh.add_face(v1, Index(vt-2), vt) );
             fv3.push_back( mesh.add_face(Index(vt-2), Index(vb-2), vt) );
             fv3.push_back( mesh.add_face(vb, vt, Index(vb-2)) );
         }

    }
    fv1.push_back(mesh.add_face(Index(0), Index(2), vb) );
    fv2.push_back(mesh.add_face(Index(1), vt, Index(3) ) );

    fv3.push_back(mesh.add_face(vt, vb, Index(3))       );
    fv3.push_back(mesh.add_face(Index(2), Index(3), vb) );

   
    double edge_length = std::min(r1,r0)/10.0; 
    CGAL::Polygon_mesh_processing::isotropic_remeshing(fv1,
                              edge_length,
                              mesh,
                              CGAL::Polygon_mesh_processing::parameters::number_of_iterations(3)
                              .protect_constraints(true));

    CGAL::Polygon_mesh_processing::isotropic_remeshing(fv2,
                              edge_length ,
                              mesh,
                              CGAL::Polygon_mesh_processing::parameters::number_of_iterations(3)
           
                              .protect_constraints(true));
    CGAL::Polygon_mesh_processing::isotropic_remeshing(fv3,
                              edge_length,
                              mesh,
                              CGAL::Polygon_mesh_processing::parameters::number_of_iterations(3)
                              .protect_constraints(true));


}

// FIXME
struct sphere_wrapper{
        
     public:
        static double radius;
        static double x0;
        static double y0;
        static double z0;
        static double function(double x, double y , double z) { return  (x-x0)*(x -x0) +  (y-y0)*(y -y0) + (z-z0)*(z -z0) -radius*radius; }
};


double sphere_wrapper::radius = 0;
double sphere_wrapper::x0 = 0;
double sphere_wrapper::y0 = 0;
double sphere_wrapper::z0 = 0;


void Surface::make_sphere( double x0, double y0, double  z0,double r0) 
{
  // TODO:: Add resolution/ edge size 
  sphere_wrapper sphere;
   
  sphere.radius = r0;
  sphere.x0=x0;
  sphere.y0=y0;
  sphere.z0=z0;
  surface_mesher(mesh,sphere.function,x0,y0,z0,r0,30,r0/5,r0/5);   

}
//

void Surface::split_edges(double  target_edge_length){
     CGAL::Polygon_mesh_processing::split_long_edges(edges(mesh), target_edge_length,mesh);
}




void Surface::save(const std::string outpath)
{    
     std::string extension = outpath.substr(outpath.find_last_of(".")+1);
     std::ofstream out(outpath);
     if ( extension=="off")
     {
        out << mesh;
        out.close();
     } 
     else if ( extension=="stl")
     {
       write_STL(outpath);
     }
}






void Surface::reconstruct( double sm_angle,
                               double sm_radius,
                               double sm_distance,
                               double approximation_ratio,
                               double average_spacing_ratio ){ 
     poisson_reconstruction(mesh,sm_angle,
                            sm_radius,
                            sm_distance,
                            approximation_ratio,
                            average_spacing_ratio);
}


void Surface::seperate_narrow_gaps() 
{

   std::map<vertex_descriptor,double> results;
   Vertex_point_pmap vppmap = get(CGAL::vertex_point,mesh);

   Tree tree(vertices(mesh).begin(),
            vertices(mesh).end(),
            Splitter(),
            Traits(vppmap)
   );

   Distance tr_dist(vppmap);

   FT distance, edgeL;
   Point_3 closest;
   bool flag;

   for (vertex_descriptor v_it : mesh.vertices())
   {
        flag = true;
    
        K_neighbor_search search(tree, mesh.point(v_it), 2,0,true,tr_dist); //tree.closest_point(point_query); must be second closest
        
        closest = mesh.point((search.begin()+1)->first); // The closest point is the query point, therefore choose second point as closest. 
        
        Point_3 current = mesh.point(v_it);
        distance = CGAL::squared_distance(current, closest );
        CGAL::Vertex_around_target_circulator<Mesh> vbegin(mesh.halfedge(v_it),mesh), done(vbegin);
        do
        {
           edgeL = CGAL::squared_distance(current, mesh.point(*vbegin) ); 
           if ( distance >= edgeL ) 
           {
              flag=false;
            break;
           }
         *vbegin++;
        }while(vbegin!=done);
    
        if (flag){results[v_it] = -0.33*static_cast<double>(CGAL::sqrt(distance)) ;} // both vertices are affected changes, used 0.5
   
   }

   adjusting_boundary_region(results.begin(),results.end()); 
   smooth_taubin(2);
   
}
Surface::vertex_vector Surface::get_close_points(Surface &other) 
{
   Surface::vertex_vector results;
   Vertex_point_pmap vppmap = get(CGAL::vertex_point,other.get_mesh());  

   Tree tree(vertices(other.get_mesh()).begin(),
            vertices(other.get_mesh()).end(),
            Splitter(),
            Traits(vppmap)
   );
   Distance tr_dist(vppmap);
   FT distance, edgeL;
   Point_3 closest;
   bool flag;
   for (vertex_descriptor v_it : mesh.vertices())
   {
        flag = true;
        K_neighbor_search search(tree, mesh.point(v_it), 2,0,true,tr_dist); 
        
        closest = other.get_mesh().point((search.begin())->first);
        
        Point_3 current = mesh.point(v_it);
        distance = CGAL::squared_distance(current, closest );
        CGAL::Vertex_around_target_circulator<Mesh> vbegin(mesh.halfedge(v_it),mesh), done(vbegin);
        do
        {
           edgeL = CGAL::squared_distance(current, mesh.point(*vbegin) ); 
           if ( distance >= edgeL ) 
           {
              flag=false;
            break;
           }
         *vbegin++;
        }while(vbegin!=done);
    
        if (flag){ results.push_back(v_it) ;} // both vertices are affected
   }
 
   return results;
   
}
//TODO rename
std::map<Surface::vertex_descriptor,double> Surface::seperate_close_surfaces(Surface& other) // TODO: based on distance to surface
{
   std::map<vertex_descriptor,double> results;
   Vertex_point_pmap vppmap = get(CGAL::vertex_point,other.get_mesh());
   Tree tree(vertices(other.get_mesh()).begin(),
            vertices(other.get_mesh()).end(),
            Splitter(),
            Traits(vppmap)
   );

   Distance tr_dist(vppmap);

   FT distance,edgeL;
   Point_3 closest;
   bool flag;
   for (vertex_descriptor v_it : mesh.vertices())
   {
        flag = true;
    
        K_neighbor_search search(tree, mesh.point(v_it), 2,0,true,tr_dist); 
        
        closest = mesh.point((search.begin()+1)->first); // The closest point is the query point, therefore choose second point as closest. 
        
        Point_3 current = mesh.point(v_it);
        distance = CGAL::squared_distance(current, closest );
        CGAL::Vertex_around_target_circulator<Mesh> vbegin(mesh.halfedge(v_it),mesh), done(vbegin);
        do
        {
           edgeL = CGAL::squared_distance(current, mesh.point(*vbegin) ); 
           if ( distance >= edgeL ) 
           {
              flag=false;
            break;
           }
         *vbegin++;
        }while(vbegin!=done);
    
        if (flag){results[v_it] = -0.33*static_cast<double>(CGAL::sqrt(distance)) ;} // both vertices are affected
   
   }
   return  results;
   
}

#endif
