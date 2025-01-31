//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_IMAGE_VIEW_HPP
#define BOOST_GIL_IMAGE_VIEW_HPP

#include <boost/gil/dynamic_step.hpp>
#include <boost/gil/iterator_from_2d.hpp>

#include <cstddef>
#include <iterator>

namespace boost { namespace gil {

////////////////////////////////////////////////////////////////////////////////////////
/// \class image_view
/// \ingroup ImageViewModel PixelBasedModel
/// \brief A lightweight object that interprets memory as a 2D array of pixels. Models ImageViewConcept,PixelBasedConcept,HasDynamicXStepTypeConcept,HasDynamicYStepTypeConcept,HasTransposedTypeConcept
///
/// Image view consists of a pixel 2D locator (defining the mechanism for navigating in 2D)
/// and the image dimensions.
///
/// Image views to images are what ranges are to STL containers. They are lightweight objects,
/// that don't own the pixels. It is the user's responsibility that the underlying data remains
/// valid for the lifetime of the image view.
///
/// Similar to iterators and ranges, constness of views does not extend to constness of pixels.
/// A const \p image_view does not allow changing its location in memory (resizing, moving) but does
/// not prevent one from changing the pixels. The latter requires an image view whose value_type
/// is const.
///
/// Images have interfaces consistent with STL 1D random access containers, so they can be used
/// directly in STL algorithms like:
/// \code
///  std::fill(img.begin(), img.end(), red_pixel);
/// \endcode
///
/// In addition, horizontal, vertical and 2D random access iterators are provided.
///
/// Note also that \p image_view does not require that its element type be a pixel. It could be
/// instantiated with a locator whose \p value_type models only \p Regular. In this case the image
/// view models the weaker RandomAccess2DImageViewConcept, and does not model PixelBasedConcept.
/// Many generic algorithms don't require the elements to be pixels.
///
////////////////////////////////////////////////////////////////////////////////////////
template <typename Loc>     // Models 2D Pixel Locator
class image_view {
public:

// aliases required by ConstRandomAccessNDImageViewConcept
    static const std::size_t num_dimensions=2;
    using value_type = typename Loc::value_type;
    using reference = typename Loc::reference;       // result of dereferencing
    using coord_t = typename Loc::coord_t;      // 1D difference type (same for all dimensions)
    using difference_type = coord_t; // result of operator-(1d_iterator,1d_iterator)
    using point_t = typename Loc::point_t;
    using locator = Loc;
    using const_t = image_view<typename Loc::const_t>;      // same as this type, but over const values
    template <std::size_t D> struct axis
    {
        using coord_t = typename Loc::template axis<D>::coord_t; // difference_type along each dimension
        using iterator = typename Loc::template axis<D>::iterator; // 1D iterator type along each dimension
    };
    using iterator = iterator_from_2d<Loc>;       // 1D iterator type for each pixel left-to-right inside top-to-bottom
    using const_iterator = typename const_t::iterator;  // may be used to examine, but not to modify values
    using const_reference = typename const_t::reference; // behaves as a const reference
    using pointer = typename std::iterator_traits<iterator>::pointer; // behaves as a pointer to the value type
    using reverse_iterator = std::reverse_iterator<iterator>;
    using size_type = std::size_t;

// aliases required by ConstRandomAccess2DImageViewConcept
    using xy_locator = locator;
    using x_iterator = typename xy_locator::x_iterator;     // pixel iterator along a row
    using y_iterator = typename xy_locator::y_iterator;     // pixel iterator along a column
    using x_coord_t = typename xy_locator::x_coord_t;
    using y_coord_t = typename xy_locator::y_coord_t;

    template <typename Deref> struct add_deref
    {
        using type = image_view<typename Loc::template add_deref<Deref>::type>;
        static type make(const image_view<Loc>& iv, const Deref& d)
        {
            return type(iv.dimensions(), Loc::template add_deref<Deref>::make(iv.pixels(),d));
        }
    };

    image_view() : _dimensions(0,0) {}
    image_view(image_view const& img_view) : _dimensions(img_view.dimensions()),  _pixels(img_view.pixels()) {}
    template <typename View> image_view(const View& iv)                                    : _dimensions(iv.dimensions()), _pixels(iv.pixels()) {}

    template <typename L2> image_view(const point_t& sz            , const L2& loc)        : _dimensions(sz),          _pixels(loc) {}
    template <typename L2> image_view(coord_t width, coord_t height, const L2& loc)        : _dimensions(x_coord_t(width),y_coord_t(height)), _pixels(loc) {}

    template <typename View> image_view& operator=(const View& iv)  { _pixels=iv.pixels(); _dimensions=iv.dimensions(); return *this; }
    image_view& operator=(const image_view& iv)                     { _pixels=iv.pixels(); _dimensions=iv.dimensions(); return *this; }

    template <typename View> bool operator==(const View& v) const   { return pixels()==v.pixels() && dimensions()==v.dimensions(); }
    template <typename View> bool operator!=(const View& v) const   { return !(*this==v); }

    template <typename L2> friend void swap(image_view<L2>& x, image_view<L2>& y);

    /// \brief Exchanges the elements of the current view with those of \a other
    ///       in constant time.
    ///
    /// \note Required by the Collection concept
    /// \see  https://www.boost.org/libs/utility/Collection.html
    void swap(image_view<Loc>& other)
    {
        using boost::gil::swap;
        swap(*this, other);
    }

    /// \brief Returns true if the view has no elements, false otherwise.
    ///
    /// \note Required by the Collection concept
    /// \see  https://www.boost.org/libs/utility/Collection.html
    bool empty() const { return !(width() > 0 && height() > 0); }

    /// \brief Returns a reference to the first element in raster order.
    ///
    /// \note Required by the ForwardCollection, since view model the concept.
    /// \see  https://www.boost.org/libs/utility/Collection.html
    reference front() const { return *begin(); }

    /// \brief Returns a reference to the last element in raster order.
    ///
    /// \note Required by the ForwardCollection, since view model the concept.
    /// \see  https://www.boost.org/libs/utility/Collection.html
    reference back() const { return *rbegin(); }

    const point_t&   dimensions()            const { return _dimensions; }
    const locator&   pixels()                const { return _pixels; }
    x_coord_t        width()                 const { return dimensions().x; }
    y_coord_t        height()                const { return dimensions().y; }
    std::size_t      num_channels()          const { return gil::num_channels<value_type>::value; }
    bool             is_1d_traversable()     const { return _pixels.is_1d_traversable(width()); }

    //\{@
    /// \name 1D navigation
    size_type           size()               const { return width()*height(); }
    iterator            begin()              const { return iterator(_pixels,_dimensions.x); }
    iterator            end()                const { return begin()+(difference_type)size(); }    // potential performance problem!
    reverse_iterator    rbegin()             const { return reverse_iterator(end()); }
    reverse_iterator    rend()               const { return reverse_iterator(begin()); }
    reference operator[](difference_type i)  const { return begin()[i]; }        // potential performance problem!
    iterator            at(difference_type i)const { return begin()+i; }
    iterator            at(const point_t& p) const { return begin()+p.y*width()+p.x; }
    iterator            at(x_coord_t x, y_coord_t y)const { return begin()+y*width()+x; }

    //\}@

    //\{@
    /// \name 2-D navigation
    reference operator()(const point_t& p)        const { return _pixels(p.x,p.y); }
    reference operator()(x_coord_t x, y_coord_t y)const { return _pixels(x,y); }
    template <std::size_t D> typename axis<D>::iterator axis_iterator(const point_t& p) const { return _pixels.template axis_iterator<D>(p); }
    xy_locator xy_at(x_coord_t x, y_coord_t y)    const { return _pixels+point_t(x_coord_t(x),y_coord_t(y)); }
    locator    xy_at(const point_t& p)            const { return _pixels+p; }
    //\}@

    //\{@
    /// \name X navigation
    x_iterator x_at(x_coord_t x, y_coord_t y)     const { return _pixels.x_at(x,y); }
    x_iterator x_at(const point_t& p)             const { return _pixels.x_at(p); }
    x_iterator row_begin(y_coord_t y)             const { return x_at(0,y); }
    x_iterator row_end(y_coord_t y)               const { return x_at(width(),y); }
    //\}@

    //\{@
    /// \name Y navigation
    y_iterator y_at(x_coord_t x, y_coord_t y)     const { return xy_at(x,y).y(); }
    y_iterator y_at(const point_t& p)             const { return xy_at(p).y(); }
    y_iterator col_begin(x_coord_t x)             const { return y_at(x,0); }
    y_iterator col_end(x_coord_t x)               const { return y_at(x,height()); }
    //\}@

private:
    template <typename L2> friend class image_view;

    point_t    _dimensions;
    xy_locator _pixels;
};

template <typename L2>
inline void swap(image_view<L2>& x, image_view<L2>& y) {
    using std::swap;
    swap(x._dimensions,y._dimensions);
    swap(x._pixels, y._pixels);            // TODO: Extend further
}

/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template <typename L>
struct channel_type<image_view<L> > : public channel_type<L> {};

template <typename L>
struct color_space_type<image_view<L> > : public color_space_type<L> {};

template <typename L>
struct channel_mapping_type<image_view<L> > : public channel_mapping_type<L> {};

template <typename L>
struct is_planar<image_view<L> > : public is_planar<L> {};

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template <typename L>
struct dynamic_x_step_type<image_view<L>>
{
    using type = image_view<typename gil::dynamic_x_step_type<L>::type>;
};

/////////////////////////////
//  HasDynamicYStepTypeConcept
/////////////////////////////

template <typename L>
struct dynamic_y_step_type<image_view<L>>
{
    using type = image_view<typename gil::dynamic_y_step_type<L>::type>;
};

/////////////////////////////
//  HasTransposedTypeConcept
/////////////////////////////

template <typename L>
struct transposed_type<image_view<L>>
{
    using type = image_view<typename transposed_type<L>::type>;
};

}}  // namespace boost::gil

#endif
