#ifndef __SURFACE_MESHER_H

#define __SURFACE_MESHER_H



#include <CGAL/Surface_mesh_traits_generator_3.h>
#include <CGAL/make_surface_mesh.h>
#include <CGAL/Implicit_surface_3.h>
#include <CGAL/Surface_mesh_default_criteria_3.h>
#include <CGAL/IO/Complex_2_in_triangulation_3_file_writer.h>


//#include <fstream>
//#include <CGAL/Point_3.h>

#include <CGAL/Surface_mesh.h>
#include <CGAL/Side_of_triangle_mesh.h>
#include <CGAL/IO/Polyhedron_iostream.h>

// needed
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h> // or excat ?
#include <CGAL/IO/output_surface_facets_to_polyhedron.h>
#include <CGAL/Mesh_polyhedron_3.h>
#include <CGAL/Surface_mesh_triangulation_generator_3.h>
#include <CGAL/Surface_mesh_default_triangulation_3.h>
#include <CGAL/Complex_2_in_triangulation_3.h>
#include <CGAL/boost/graph/copy_face_graph.h>


template <typename FT, typename P>
class FT_to_point_function_wrapper : public std::unary_function<P, FT>
{
  typedef FT (*Implicit_function)(FT, FT, FT);
  Implicit_function function;
public:
  typedef P Point;
  FT_to_point_function_wrapper(Implicit_function f) : function(f) {}
  FT operator()(Point p) const { return function(p.x(), p.y(), p.z()); }

};


template<typename Mesh , typename Implicit_Function> // Requires origin  
void surface_mesher(Mesh& mesh, Implicit_Function func, double& x0 ,double& y0, double& z0 , double bounding_sphere_radius, double angular_bound, double radius_bound, double distance_bound ) // remove implivit anf center use CEnter
{
    typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
    typedef CGAL::Surface_mesh_triangulation_generator_3<Kernel>::Type Tr;
    //typedef CGAL::Surface_mesh_default_triangulation_3 Tr; // Surface_mesh_default
    typedef CGAL::Complex_2_in_triangulation_3<Tr> C2t3;
    typedef Tr::Geom_traits GT;
    typedef Kernel::Sphere_3 Sphere_3;
    typedef Kernel::Point_3 Point_3;
    typedef Kernel::FT FT;

    typedef CGAL::Mesh_polyhedron_3<Kernel>::type Polyhedron;

    typedef FT_to_point_function_wrapper<FT, Point_3> Function;
    typedef CGAL::Implicit_surface_3<Kernel, Function> Surface_3;

    // no known conversion for argument 1 from ‘CGAL::Point_3<CGAL::Epeck>’ to ‘const Point_3_& {aka const CGAL::Point_3<CGAL::Epick>&}’
    Tr tr;
    C2t3 c2t3 (tr);
    Polyhedron poly;
    Function wrapper(func);
    GT::Point_3 bounding_sphere_center(x0, y0, z0);
    GT::FT bounding_sphere_squared_radius = bounding_sphere_radius*bounding_sphere_radius*2;
    GT::Sphere_3 bounding_sphere(bounding_sphere_center,bounding_sphere_squared_radius);

    Surface_3 surface(wrapper,bounding_sphere,1.0e-5 ); 

    CGAL::Surface_mesh_default_criteria_3<Tr> criteria(angular_bound,radius_bound,distance_bound);

    CGAL::make_surface_mesh(c2t3, surface, criteria, CGAL::Non_manifold_tag());

    CGAL::output_surface_facets_to_polyhedron(c2t3, poly);

    CGAL::copy_face_graph(poly,mesh);

}
template<typename Mesh , typename Implicit_Function> // Requires origin  
void surface_mesher(Mesh& mesh, Implicit_Function func,  double bounding_sphere_radius, double angular_bound, double radius_bound, double distance_bound )
{
    typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
    typedef CGAL::Surface_mesh_triangulation_generator_3<Kernel>::Type Tr;
    //typedef CGAL::Surface_mesh_default_triangulation_3 Tr; // Surface_mesh_default
    typedef CGAL::Complex_2_in_triangulation_3<Tr> C2t3;
    //typedef Tr::Geom_traits GT;
    typedef Kernel::Sphere_3 Sphere_3;
    typedef Kernel::Point_3 Point_3;
    typedef Kernel::FT FT;
    //Surface_mesh_traits().construct_initial_points_object()(surface_of_sphere_2, CGAL::inserter(tr_3), initial_number_of_points)
    typedef CGAL::Mesh_polyhedron_3<Kernel>::type Polyhedron;

    typedef FT_to_point_function_wrapper<FT, Point_3> Function;
    typedef CGAL::Implicit_surface_3<Kernel, Function> Surface_3;


    Tr tr;
    C2t3 c2t3 (tr);
    Polyhedron poly;
    Function wrapper(func);
    //

    if ( bounding_sphere_radius< 1.0)
    {
       bounding_sphere_radius = 1.0; // problem with squared < 1
    }
    Surface_3 surface(wrapper,Sphere_3(CGAL::ORIGIN, bounding_sphere_radius*bounding_sphere_radius),1.0e-5 ); 

    CGAL::Surface_mesh_default_criteria_3<Tr> criteria(angular_bound,radius_bound,distance_bound);


//    CGAL::make_surface_mesh(c2t3, surface, criteria,CGAL::Manifold_with_boundary_tag());
    CGAL::make_surface_mesh(c2t3, surface, criteria, CGAL::Non_manifold_tag());

    CGAL::output_surface_facets_to_polyhedron(c2t3, poly);


    CGAL::copy_face_graph(poly,mesh);

}


#endif