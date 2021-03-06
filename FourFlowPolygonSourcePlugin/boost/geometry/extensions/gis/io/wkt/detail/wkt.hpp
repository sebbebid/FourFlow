// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright Barend Gehrels 2008-2009, Geodan, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_IO_WKT_DETAIL_WKT_HPP
#define BOOST_GEOMETRY_IO_WKT_DETAIL_WKT_HPP


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace wkt {


struct prefix_point
{
    static inline const char* apply() { return "POINT"; }
};

struct prefix_polygon
{
    static inline const char* apply() { return "POLYGON"; }
};

struct prefix_linestring
{
    static inline const char* apply() { return "LINESTRING"; }
};



}} // namespace wkt::impl
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_IO_WKT_DETAIL_WKT_HPP
