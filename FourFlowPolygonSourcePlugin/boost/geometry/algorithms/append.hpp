// Boost.Geometry (aka GGL, Generic Geometry Library)
//
// Copyright Barend Gehrels 2007-2009, Geodan, Amsterdam, the Netherlands.
// Copyright Bruno Lalande 2008, 2009
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_APPEND_HPP
#define BOOST_GEOMETRY_ALGORITHMS_APPEND_HPP

#include <boost/range/functions.hpp>
#include <boost/range/metafunctions.hpp>
#include <boost/type_traits/remove_const.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/copy.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace append {

template <typename Geometry, typename Point, bool UseStd>
struct append_point {};

template <typename Geometry, typename Point>
struct append_point<Geometry, Point, true>
{
    static inline void apply(Geometry& geometry, Point const& point, int , int )
    {
        typename point_type<Geometry>::type point_type;

        copy_coordinates(point, point_type);
        geometry.push_back(point_type);
    }
};

template <typename Geometry, typename Point>
struct append_point<Geometry, Point, false>
{
    static inline void apply(Geometry& geometry, Point const& point,
                int ring_index, int multi_index)
    {
        traits::append_point<Geometry, Point>::apply(geometry, point,
                    ring_index, multi_index);
    }
};

template <typename Geometry, typename Range, bool UseStd>
struct append_range
{
    typedef typename boost::range_value<Range>::type point_type;

    static inline void apply(Geometry& geometry, Range const& range,
                int ring_index, int multi_index)
    {
        for (typename boost::range_const_iterator<Range>::type
            it = boost::begin(range);
             it != boost::end(range);
             ++it)
        {
            append_point<Geometry, point_type, UseStd>::apply(geometry, *it,
                        ring_index, multi_index);
        }
    }
};

template <typename Polygon, typename Point, bool Std>
struct point_to_poly
{
    typedef typename ring_type<Polygon>::type range_type;

    static inline void apply(Polygon& polygon, Point const& point,
                int ring_index, int )
    {
        if (ring_index == -1)
        {
            append_point<range_type, Point, Std>::apply(
                        exterior_ring(polygon), point, -1, -1);
        }
        else if (ring_index < int(num_interior_rings(polygon)))
        {
            append_point<range_type, Point, Std>::apply(
                        interior_rings(polygon)[ring_index], point, -1, -1);
        }
    }
};

template <typename Polygon, typename Range, bool Std>
struct range_to_poly
{
    typedef typename ring_type<Polygon>::type ring_type;

    static inline void apply(Polygon& polygon, Range const& range,
                int ring_index, int )
    {
        if (ring_index == -1)
        {
            append_range<ring_type, Range, Std>::apply(
                        exterior_ring(polygon), range, -1, -1);
        }
        else if (ring_index < int(num_interior_rings(polygon)))
        {
            append_range<ring_type, Range, Std>::apply(
                        interior_rings(polygon)[ring_index], range, -1, -1);
        }
    }
};

}} // namespace detail::append
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// (RoP = range or point, Std = use std library)

// Default case (where RoP will be range/array/etc)
template <typename Tag, typename TagRoP, typename G, typename RoP, bool Std>
struct append : detail::append::append_range<G, RoP, Std> {};

// Append a point to any geometry
template <typename Tag, typename G, typename P, bool Std>
struct append<Tag, point_tag, G, P, Std>
    : detail::append::append_point<G, P, Std> {};

// Never possible to append anything to a point/box/n-sphere
template <typename TagRoP, typename Point, typename RoP, bool Std>
struct append<point_tag, TagRoP, Point, RoP, Std> {};

template <typename TagRoP, typename Box, typename RoP, bool Std>
struct append<box_tag, TagRoP, Box, RoP, Std> {};


template <typename Polygon, typename TagRange, typename Range, bool Std>
struct append<polygon_tag, TagRange, Polygon, Range, Std>
        : detail::append::range_to_poly<Polygon, Range, Std> {};

template <typename Polygon, typename Point, bool Std>
struct append<polygon_tag, point_tag, Polygon, Point, Std>
        : detail::append::point_to_poly<Polygon, Point, Std> {};

// Multi-linestring and multi-polygon might either implement traits
// or use standard...

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
    \brief Appends one or more points to a linestring, ring, polygon, multi
    \ingroup access
    \param geometry a geometry
    \param range_or_point the point or range to add
    \param ring_index the index of the ring in case of a polygon:
        exterior ring (-1, the default) or  interior ring index
    \param multi_index reserved for multi polygons
 */
template <typename Geometry, typename RoP>
inline void append(Geometry& geometry, RoP const& range_or_point,
            int ring_index = -1, int multi_index = 0)
{
    concept::check<Geometry>();

    typedef typename boost::remove_const<Geometry>::type ncg_type;

    dispatch::append
        <
            typename tag<Geometry>::type,
            typename tag<RoP>::type,
            ncg_type,
            RoP,
            traits::use_std<ncg_type>::value
        >::apply(geometry, range_or_point, ring_index, multi_index);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_APPEND_HPP
