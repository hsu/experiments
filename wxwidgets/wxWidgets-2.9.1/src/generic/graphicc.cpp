/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/graphicc.cpp
// Purpose:     cairo device context class
// Author:      Stefan Csomor
// Modified by:
// Created:     2006-10-03
// RCS-ID:      $Id$
// Copyright:   (c) 2006 Stefan Csomor
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_GRAPHICS_CONTEXT

#include "wx/graphics.h"

#if wxUSE_CAIRO

#include "wx/cairo.h"

#ifndef WX_PRECOMP
    #include "wx/bitmap.h"
    #include "wx/icon.h"
    #include "wx/dcclient.h"
    #include "wx/dcmemory.h"
    #include "wx/dcprint.h"
#endif

#include "wx/private/graphics.h"
#include "wx/rawbmp.h"
#include "wx/vector.h"

using namespace std;

//-----------------------------------------------------------------------------
// device context implementation
//
// more and more of the dc functionality should be implemented by calling
// the appropricate wxCairoContext, but we will have to do that step by step
// also coordinate conversions should be moved to native matrix ops
//-----------------------------------------------------------------------------

// we always stock two context states, one at entry, to be able to preserve the
// state we were called with, the other one after changing to HI Graphics orientation
// (this one is used for getting back clippings etc)

//-----------------------------------------------------------------------------
// wxGraphicsPath implementation
//-----------------------------------------------------------------------------

// TODO remove this dependency (gdiplus needs the macros)

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#include <cairo.h>
#ifdef __WXMSW__
#include <cairo-win32.h>
#endif

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include "wx/fontutil.h"
#include "wx/gtk/dc.h"
#endif

#ifdef __WXMSW__
#include <cairo-win32.h>
#endif

#ifdef __WXMAC__
#include "wx/osx/private.h"
#include <cairo-quartz.h>
#include <cairo-atsui.h>
#endif

class WXDLLIMPEXP_CORE wxCairoPathData : public wxGraphicsPathData
{
public :
    wxCairoPathData(wxGraphicsRenderer* renderer, cairo_t* path = NULL);
    ~wxCairoPathData();

    virtual wxGraphicsObjectRefData *Clone() const;

    //
    // These are the path primitives from which everything else can be constructed
    //

    // begins a new subpath at (x,y)
    virtual void MoveToPoint( wxDouble x, wxDouble y );

    // adds a straight line from the current point to (x,y)
    virtual void AddLineToPoint( wxDouble x, wxDouble y );

    // adds a cubic Bezier curve from the current point, using two control points and an end point
    virtual void AddCurveToPoint( wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y );


    // adds an arc of a circle centering at (x,y) with radius (r) from startAngle to endAngle
    virtual void AddArc( wxDouble x, wxDouble y, wxDouble r, wxDouble startAngle, wxDouble endAngle, bool clockwise ) ;

    // gets the last point of the current path, (0,0) if not yet set
    virtual void GetCurrentPoint( wxDouble* x, wxDouble* y) const;

    // adds another path
    virtual void AddPath( const wxGraphicsPathData* path );

    // closes the current sub-path
    virtual void CloseSubpath();

    //
    // These are convenience functions which - if not available natively will be assembled
    // using the primitives from above
    //

    /*

    // appends a rectangle as a new closed subpath
    virtual void AddRectangle( wxDouble x, wxDouble y, wxDouble w, wxDouble h ) ;
    // appends an ellipsis as a new closed subpath fitting the passed rectangle
    virtual void AddEllipsis( wxDouble x, wxDouble y, wxDouble w , wxDouble h ) ;

    // draws a an arc to two tangents connecting (current) to (x1,y1) and (x1,y1) to (x2,y2), also a straight line from (current) to (x1,y1)
    virtual void AddArcToPoint( wxDouble x1, wxDouble y1 , wxDouble x2, wxDouble y2, wxDouble r )  ;
    */

    // returns the native path
    virtual void * GetNativePath() const ;

    // give the native path returned by GetNativePath() back (there might be some deallocations necessary)
    virtual void UnGetNativePath(void *p) const;

    // transforms each point of this path by the matrix
    virtual void Transform( const wxGraphicsMatrixData* matrix ) ;

    // gets the bounding box enclosing all points (possibly including control points)
    virtual void GetBox(wxDouble *x, wxDouble *y, wxDouble *w, wxDouble *h) const;

    virtual bool Contains( wxDouble x, wxDouble y, wxPolygonFillMode fillStyle = wxWINDING_RULE) const;

private :
    cairo_t* m_pathContext;
};

class WXDLLIMPEXP_CORE wxCairoMatrixData : public wxGraphicsMatrixData
{
public :
    wxCairoMatrixData(wxGraphicsRenderer* renderer, const cairo_matrix_t* matrix = NULL ) ;
    virtual ~wxCairoMatrixData() ;

    virtual wxGraphicsObjectRefData *Clone() const ;

    // concatenates the matrix
    virtual void Concat( const wxGraphicsMatrixData *t );

    // sets the matrix to the respective values
    virtual void Set(wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0);

    // gets the component valuess of the matrix
    virtual void Get(wxDouble* a=NULL, wxDouble* b=NULL,  wxDouble* c=NULL,
                     wxDouble* d=NULL, wxDouble* tx=NULL, wxDouble* ty=NULL) const;

    // makes this the inverse matrix
    virtual void Invert();

    // returns true if the elements of the transformation matrix are equal ?
    virtual bool IsEqual( const wxGraphicsMatrixData* t) const ;

    // return true if this is the identity matrix
    virtual bool IsIdentity() const;

    //
    // transformation
    //

    // add the translation to this matrix
    virtual void Translate( wxDouble dx , wxDouble dy );

    // add the scale to this matrix
    virtual void Scale( wxDouble xScale , wxDouble yScale );

    // add the rotation to this matrix (radians)
    virtual void Rotate( wxDouble angle );

    //
    // apply the transforms
    //

    // applies that matrix to the point
    virtual void TransformPoint( wxDouble *x, wxDouble *y ) const;

    // applies the matrix except for translations
    virtual void TransformDistance( wxDouble *dx, wxDouble *dy ) const;

    // returns the native representation
    virtual void * GetNativeMatrix() const;
private:
    cairo_matrix_t m_matrix ;
} ;

class WXDLLIMPEXP_CORE wxCairoPenData : public wxGraphicsObjectRefData
{
public:
    wxCairoPenData( wxGraphicsRenderer* renderer, const wxPen &pen );
    ~wxCairoPenData();

    void Init();

    virtual void Apply( wxGraphicsContext* context );
    virtual wxDouble GetWidth() { return m_width; }

private :
    double m_width;

    double m_red;
    double m_green;
    double m_blue;
    double m_alpha;

    cairo_line_cap_t m_cap;
    cairo_line_join_t m_join;

    int m_count;
    const double *m_lengths;
    double *m_userLengths;

    wxPen m_pen;

    wxDECLARE_NO_COPY_CLASS(wxCairoPenData);
};

class WXDLLIMPEXP_CORE wxCairoBrushData : public wxGraphicsObjectRefData
{
public:
    wxCairoBrushData( wxGraphicsRenderer* renderer );
    wxCairoBrushData( wxGraphicsRenderer* renderer, const wxBrush &brush );
    ~wxCairoBrushData ();

    virtual void Apply( wxGraphicsContext* context );

    void CreateLinearGradientBrush(wxDouble x1, wxDouble y1,
                                   wxDouble x2, wxDouble y2,
                                   const wxGraphicsGradientStops& stops);
    void CreateRadialGradientBrush(wxDouble xo, wxDouble yo,
                                   wxDouble xc, wxDouble yc, wxDouble radius,
                                   const wxGraphicsGradientStops& stops);

protected:
    virtual void Init();

    // common part of Create{Linear,Radial}GradientBrush()
    void AddGradientStops(const wxGraphicsGradientStops& stops);

private :
    double m_red;
    double m_green;
    double m_blue;
    double m_alpha;

    cairo_pattern_t* m_brushPattern;
};

class wxCairoFontData : public wxGraphicsObjectRefData
{
public:
    wxCairoFontData( wxGraphicsRenderer* renderer, const wxFont &font, const wxColour& col );
    ~wxCairoFontData();

    virtual void Apply( wxGraphicsContext* context );
#ifdef __WXGTK__
    const PangoFontDescription* GetFont() const { return m_font; }
    bool GetUnderlined() const { return m_underlined; }
#endif
private :
    double m_size;
    bool m_underlined;
    double m_red;
    double m_green;
    double m_blue;
    double m_alpha;
#ifdef __WXMAC__
    cairo_font_face_t *m_font;
#elif defined(__WXGTK__)
    PangoFontDescription* m_font;
#else
    wxCharBuffer m_fontName;
    cairo_font_slant_t m_slant;
    cairo_font_weight_t m_weight;
#endif
#ifdef __WXMSW__
    wxCairoContext( wxGraphicsRenderer* renderer, HDC context );
#endif
};

class wxCairoBitmapData : public wxGraphicsObjectRefData
{
public:
    wxCairoBitmapData( wxGraphicsRenderer* renderer, const wxBitmap& bmp );
    wxCairoBitmapData( wxGraphicsRenderer* renderer, cairo_surface_t* bitmap );
    ~wxCairoBitmapData();

    virtual cairo_surface_t* GetCairoSurface() { return m_surface; }
    virtual cairo_pattern_t* GetCairoPattern() { return m_pattern; }
    virtual wxSize GetSize() { return wxSize(m_width, m_height); }
private :
    cairo_surface_t* m_surface;
    cairo_pattern_t* m_pattern;
    int m_width;
    int m_height;
    unsigned char* m_buffer;
};

class WXDLLIMPEXP_CORE wxCairoContext : public wxGraphicsContext
{
public:
    wxCairoContext( wxGraphicsRenderer* renderer, const wxWindowDC& dc );
    wxCairoContext( wxGraphicsRenderer* renderer, const wxMemoryDC& dc );
    wxCairoContext( wxGraphicsRenderer* renderer, const wxPrinterDC& dc );
#ifdef __WXGTK__
    wxCairoContext( wxGraphicsRenderer* renderer, GdkDrawable *drawable );
#endif
    wxCairoContext( wxGraphicsRenderer* renderer, cairo_t *context );
    wxCairoContext( wxGraphicsRenderer* renderer, wxWindow *window);
    wxCairoContext();
    virtual ~wxCairoContext();

    virtual bool ShouldOffset() const
    {
        int penwidth = 0 ;
        if ( !m_pen.IsNull() )
        {
            penwidth = (int)((wxCairoPenData*)m_pen.GetRefData())->GetWidth();
            if ( penwidth == 0 )
                penwidth = 1;
        }
        return ( penwidth % 2 ) == 1;
    }

    virtual void Clip( const wxRegion &region );
#ifdef __WXMSW__
    cairo_surface_t* m_mswSurface;
#endif

    // clips drawings to the rect
    virtual void Clip( wxDouble x, wxDouble y, wxDouble w, wxDouble h );

    // resets the clipping to original extent
    virtual void ResetClip();

    virtual void * GetNativeContext();

    virtual bool SetAntialiasMode(wxAntialiasMode antialias);

    virtual bool SetCompositionMode(wxCompositionMode op);

    virtual void BeginLayer(wxDouble opacity);

    virtual void EndLayer();

    virtual void StrokePath( const wxGraphicsPath& p );
    virtual void FillPath( const wxGraphicsPath& p , wxPolygonFillMode fillStyle = wxWINDING_RULE );

    virtual void Translate( wxDouble dx , wxDouble dy );
    virtual void Scale( wxDouble xScale , wxDouble yScale );
    virtual void Rotate( wxDouble angle );

    // concatenates this transform with the current transform of this context
    virtual void ConcatTransform( const wxGraphicsMatrix& matrix );

    // sets the transform of this context
    virtual void SetTransform( const wxGraphicsMatrix& matrix );

    // gets the matrix of this context
    virtual wxGraphicsMatrix GetTransform() const;

    virtual void DrawBitmap( const wxGraphicsBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    virtual void DrawBitmap( const wxBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    virtual void DrawIcon( const wxIcon &icon, wxDouble x, wxDouble y, wxDouble w, wxDouble h );
    virtual void PushState();
    virtual void PopState();

    virtual void GetTextExtent( const wxString &str, wxDouble *width, wxDouble *height,
                                wxDouble *descent, wxDouble *externalLeading ) const;
    virtual void GetPartialTextExtents(const wxString& text, wxArrayDouble& widths) const;

protected:
    virtual void DoDrawText( const wxString &str, wxDouble x, wxDouble y );

private:
    void Init(cairo_t *context);

    cairo_t* m_context;

    wxVector<float> m_layerOpacities;

    wxDECLARE_NO_COPY_CLASS(wxCairoContext);
};

//-----------------------------------------------------------------------------
// wxCairoPenData implementation
//-----------------------------------------------------------------------------

wxCairoPenData::~wxCairoPenData()
{
    delete[] m_userLengths;
}

void wxCairoPenData::Init()
{
    m_lengths = NULL;
    m_userLengths = NULL;
    m_width = 0;
    m_count = 0;
}

wxCairoPenData::wxCairoPenData( wxGraphicsRenderer* renderer, const wxPen &pen )
: wxGraphicsObjectRefData(renderer)
{
    Init();
    m_pen = pen;
    m_width = m_pen.GetWidth();
    if (m_width <= 0.0)
        m_width = 0.1;

    m_red = m_pen.GetColour().Red()/255.0;
    m_green = m_pen.GetColour().Green()/255.0;
    m_blue = m_pen.GetColour().Blue()/255.0;
    m_alpha = m_pen.GetColour().Alpha()/255.0;

    switch ( m_pen.GetCap() )
    {
    case wxCAP_ROUND :
        m_cap = CAIRO_LINE_CAP_ROUND;
        break;

    case wxCAP_PROJECTING :
        m_cap = CAIRO_LINE_CAP_SQUARE;
        break;

    case wxCAP_BUTT :
        m_cap = CAIRO_LINE_CAP_BUTT;
        break;

    default :
        m_cap = CAIRO_LINE_CAP_BUTT;
        break;
    }

    switch ( m_pen.GetJoin() )
    {
    case wxJOIN_BEVEL :
        m_join = CAIRO_LINE_JOIN_BEVEL;
        break;

    case wxJOIN_MITER :
        m_join = CAIRO_LINE_JOIN_MITER;
        break;

    case wxJOIN_ROUND :
        m_join = CAIRO_LINE_JOIN_ROUND;
        break;

    default :
        m_join = CAIRO_LINE_JOIN_MITER;
        break;
    }

    const double dashUnit = m_width < 1.0 ? 1.0 : m_width;
    const double dotted[] =
    {
        dashUnit , dashUnit + 2.0
    };
    static const double short_dashed[] =
    {
        9.0 , 6.0
    };
    static const double dashed[] =
    {
        19.0 , 9.0
    };
    static const double dotted_dashed[] =
    {
        9.0 , 6.0 , 3.0 , 3.0
    };

    switch ( m_pen.GetStyle() )
    {
    case wxPENSTYLE_SOLID :
        break;

    case wxPENSTYLE_DOT :
        m_count = WXSIZEOF(dotted);
        m_userLengths = new double[ m_count ] ;
        memcpy( m_userLengths, dotted, sizeof(dotted) );
        m_lengths = m_userLengths;
        break;

    case wxPENSTYLE_LONG_DASH :
        m_lengths = dashed ;
        m_count = WXSIZEOF(dashed);
        break;

    case wxPENSTYLE_SHORT_DASH :
        m_lengths = short_dashed ;
        m_count = WXSIZEOF(short_dashed);
        break;

    case wxPENSTYLE_DOT_DASH :
        m_lengths = dotted_dashed ;
        m_count = WXSIZEOF(dotted_dashed);
        break;

    case wxPENSTYLE_USER_DASH :
        {
            wxDash *wxdashes ;
            m_count = m_pen.GetDashes( &wxdashes ) ;
            if ((wxdashes != NULL) && (m_count > 0))
            {
                m_userLengths = new double[m_count] ;
                for ( int i = 0 ; i < m_count ; ++i )
                {
                    m_userLengths[i] = wxdashes[i] * dashUnit ;

                    if ( i % 2 == 1 && m_userLengths[i] < dashUnit + 2.0 )
                        m_userLengths[i] = dashUnit + 2.0 ;
                    else if ( i % 2 == 0 && m_userLengths[i] < dashUnit )
                        m_userLengths[i] = dashUnit ;
                }
            }
            m_lengths = m_userLengths ;
        }
        break;
    case wxPENSTYLE_STIPPLE :
        {
            /*
            wxBitmap* bmp = pen.GetStipple();
            if ( bmp && bmp->Ok() )
            {
            wxDELETE( m_penImage );
            wxDELETE( m_penBrush );
            m_penImage = Bitmap::FromHBITMAP((HBITMAP)bmp->GetHBITMAP(),(HPALETTE)bmp->GetPalette()->GetHPALETTE());
            m_penBrush = new TextureBrush(m_penImage);
            m_pen->SetBrush( m_penBrush );
            }
            */
        }
        break;
    default :
        if ( m_pen.GetStyle() >= wxPENSTYLE_FIRST_HATCH
            && m_pen.GetStyle() <= wxPENSTYLE_LAST_HATCH )
        {
            /*
            wxDELETE( m_penBrush );
            HatchStyle style = HatchStyleHorizontal;
            switch( pen.GetStyle() )
            {
            case wxPENSTYLE_BDIAGONAL_HATCH :
            style = HatchStyleBackwardDiagonal;
            break ;
            case wxPENSTYLE_CROSSDIAG_HATCH :
            style = HatchStyleDiagonalCross;
            break ;
            case wxPENSTYLE_FDIAGONAL_HATCH :
            style = HatchStyleForwardDiagonal;
            break ;
            case wxPENSTYLE_CROSS_HATCH :
            style = HatchStyleCross;
            break ;
            case wxPENSTYLE_HORIZONTAL_HATCH :
            style = HatchStyleHorizontal;
            break ;
            case wxPENSTYLE_VERTICAL_HATCH :
            style = HatchStyleVertical;
            break ;

            }
            m_penBrush = new HatchBrush(style,Color( pen.GetColour().Alpha() , pen.GetColour().Red() ,
            pen.GetColour().Green() , pen.GetColour().Blue() ), Color.Transparent );
            m_pen->SetBrush( m_penBrush )
            */
        }
        break;
    }
}

void wxCairoPenData::Apply( wxGraphicsContext* context )
{
    cairo_t * ctext = (cairo_t*) context->GetNativeContext();
    cairo_set_line_width(ctext,m_width);
    cairo_set_source_rgba(ctext,m_red,m_green, m_blue,m_alpha);
    cairo_set_line_cap(ctext,m_cap);
    cairo_set_line_join(ctext,m_join);
    cairo_set_dash(ctext,(double*)m_lengths,m_count,0.0);
}

//-----------------------------------------------------------------------------
// wxCairoBrushData implementation
//-----------------------------------------------------------------------------

wxCairoBrushData::wxCairoBrushData( wxGraphicsRenderer* renderer )
  : wxGraphicsObjectRefData( renderer )
{
    Init();
}

wxCairoBrushData::wxCairoBrushData( wxGraphicsRenderer* renderer, const wxBrush &brush )
  : wxGraphicsObjectRefData(renderer)
{
    Init();

    m_red = brush.GetColour().Red()/255.0;
    m_green = brush.GetColour().Green()/255.0;
    m_blue = brush.GetColour().Blue()/255.0;
    m_alpha = brush.GetColour().Alpha()/255.0;
    /*
    if ( brush.GetStyle() == wxBRUSHSTYLE_SOLID)
    {
    m_brush = new SolidBrush( Color( brush.GetColour().Alpha() , brush.GetColour().Red() ,
    brush.GetColour().Green() , brush.GetColour().Blue() ) );
    }
    else if ( brush.IsHatch() )
    {
    HatchStyle style = HatchStyleHorizontal;
    switch( brush.GetStyle() )
    {
    case wxBRUSHSTYLE_BDIAGONAL_HATCH :
    style = HatchStyleBackwardDiagonal;
    break ;
    case wxBRUSHSTYLE_CROSSDIAG_HATCH :
    style = HatchStyleDiagonalCross;
    break ;
    case wxBRUSHSTYLE_FDIAGONAL_HATCH :
    style = HatchStyleForwardDiagonal;
    break ;
    case wxBRUSHSTYLE_CROSS_HATCH :
    style = HatchStyleCross;
    break ;
    case wxBRUSHSTYLE_HORIZONTAL_HATCH :
    style = HatchStyleHorizontal;
    break ;
    case wxBRUSHSTYLE_VERTICAL_HATCH :
    style = HatchStyleVertical;
    break ;

    }
    m_brush = new HatchBrush(style,Color( brush.GetColour().Alpha() , brush.GetColour().Red() ,
    brush.GetColour().Green() , brush.GetColour().Blue() ), Color.Transparent );
    }
    else
    {
    wxBitmap* bmp = brush.GetStipple();
    if ( bmp && bmp->Ok() )
    {
    wxDELETE( m_brushImage );
    m_brushImage = Bitmap::FromHBITMAP((HBITMAP)bmp->GetHBITMAP(),(HPALETTE)bmp->GetPalette()->GetHPALETTE());
    m_brush = new TextureBrush(m_brushImage);
    }
    }
    */
}

wxCairoBrushData::~wxCairoBrushData ()
{
    if (m_brushPattern)
        cairo_pattern_destroy(m_brushPattern);
}

void wxCairoBrushData::Apply( wxGraphicsContext* context )
{
    cairo_t * ctext = (cairo_t*) context->GetNativeContext();
    if ( m_brushPattern )
    {
        cairo_set_source(ctext,m_brushPattern);
    }
    else
    {
        cairo_set_source_rgba(ctext,m_red,m_green, m_blue,m_alpha);
    }
}

void wxCairoBrushData::AddGradientStops(const wxGraphicsGradientStops& stops)
{
    // loop over all the stops, they include the beginning and ending ones
    const unsigned numStops = stops.GetCount();
    for ( unsigned n = 0; n < numStops; n++ )
    {
        const wxGraphicsGradientStop stop = stops.Item(n);

        const wxColour col = stop.GetColour();

        cairo_pattern_add_color_stop_rgba
        (
            m_brushPattern,
            stop.GetPosition(),
            col.Red()/255.0,
            col.Green()/255.0,
            col.Blue()/255.0,
            col.Alpha()/255.0
        );
    }

    wxASSERT_MSG(cairo_pattern_status(m_brushPattern) == CAIRO_STATUS_SUCCESS,
                 wxT("Couldn't create cairo pattern"));
}

void
wxCairoBrushData::CreateLinearGradientBrush(wxDouble x1, wxDouble y1,
                                            wxDouble x2, wxDouble y2,
                                            const wxGraphicsGradientStops& stops)
{
    m_brushPattern = cairo_pattern_create_linear(x1,y1,x2,y2);

    AddGradientStops(stops);
}

void
wxCairoBrushData::CreateRadialGradientBrush(wxDouble xo, wxDouble yo,
                                            wxDouble xc, wxDouble yc,
                                            wxDouble radius,
                                            const wxGraphicsGradientStops& stops)
{
    m_brushPattern = cairo_pattern_create_radial(xo,yo,0.0,xc,yc,radius);

    AddGradientStops(stops);
}

void wxCairoBrushData::Init()
{
    m_brushPattern = NULL;
}

//-----------------------------------------------------------------------------
// wxCairoFontData implementation
//-----------------------------------------------------------------------------

wxCairoFontData::wxCairoFontData( wxGraphicsRenderer* renderer, const wxFont &font,
                         const wxColour& col ) : wxGraphicsObjectRefData(renderer)
{
    m_red = col.Red()/255.0;
    m_green = col.Green()/255.0;
    m_blue = col.Blue()/255.0;
    m_alpha = col.Alpha()/255.0;
    m_size = font.GetPointSize();
    m_underlined = font.GetUnderlined();

#ifdef __WXMAC__
    m_font = cairo_quartz_font_face_create_for_cgfont( font.OSXGetCGFont() );
#elif defined(__WXGTK__)
    m_font = pango_font_description_copy( font.GetNativeFontInfo()->description );
#else
    m_fontName = font.GetFaceName().mb_str(wxConvUTF8);
    m_slant = font.GetStyle() == wxFONTSTYLE_ITALIC ? CAIRO_FONT_SLANT_ITALIC:CAIRO_FONT_SLANT_NORMAL;
    m_weight = font.GetWeight() == wxFONTWEIGHT_BOLD ? CAIRO_FONT_WEIGHT_BOLD:CAIRO_FONT_WEIGHT_NORMAL;
#endif
}

wxCairoFontData::~wxCairoFontData()
{
#ifdef __WXMAC__
    cairo_font_face_destroy( m_font );
#elif defined(__WXGTK__)
    pango_font_description_free( m_font );
#else
#endif
}

void wxCairoFontData::Apply( wxGraphicsContext* context )
{
    cairo_t * ctext = (cairo_t*) context->GetNativeContext();
    cairo_set_source_rgba(ctext,m_red,m_green, m_blue,m_alpha);
#ifdef __WXGTK__
    // the rest is done using Pango layouts
#elif defined(__WXMAC__)
    cairo_set_font_face(ctext, m_font);
    cairo_set_font_size(ctext, m_size );
#else
    cairo_select_font_face(ctext, m_fontName, m_slant, m_weights );
    cairo_set_font_size(ctext, m_size );
#endif
}

//-----------------------------------------------------------------------------
// wxCairoPathData implementation
//-----------------------------------------------------------------------------

wxCairoPathData::wxCairoPathData( wxGraphicsRenderer* renderer, cairo_t* pathcontext)
    : wxGraphicsPathData(renderer)
{
    if (pathcontext)
    {
        m_pathContext = pathcontext;
    }
    else
    {
        cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,1,1);
        m_pathContext = cairo_create(surface);
        cairo_surface_destroy (surface);
    }
}

wxCairoPathData::~wxCairoPathData()
{
    cairo_destroy(m_pathContext);
}

wxGraphicsObjectRefData *wxCairoPathData::Clone() const
{
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,1,1);
    cairo_t* pathcontext = cairo_create(surface);
    cairo_surface_destroy (surface);

    cairo_path_t* path = cairo_copy_path(m_pathContext);
    cairo_append_path(pathcontext, path);
    cairo_path_destroy(path);
    return new wxCairoPathData( GetRenderer() ,pathcontext);
}


void* wxCairoPathData::GetNativePath() const
{
    return cairo_copy_path(m_pathContext) ;
}

void wxCairoPathData::UnGetNativePath(void *p) const
{
    cairo_path_destroy((cairo_path_t*)p);
}

//
// The Primitives
//

void wxCairoPathData::MoveToPoint( wxDouble x , wxDouble y )
{
    cairo_move_to(m_pathContext,x,y);
}

void wxCairoPathData::AddLineToPoint( wxDouble x , wxDouble y )
{
    cairo_line_to(m_pathContext,x,y);
}

void wxCairoPathData::AddPath( const wxGraphicsPathData* path )
{
    cairo_path_t* p = (cairo_path_t*)path->GetNativePath();
    cairo_append_path(m_pathContext, p);
    UnGetNativePath(p);
}

void wxCairoPathData::CloseSubpath()
{
    cairo_close_path(m_pathContext);
}

void wxCairoPathData::AddCurveToPoint( wxDouble cx1, wxDouble cy1, wxDouble cx2, wxDouble cy2, wxDouble x, wxDouble y )
{
    cairo_curve_to(m_pathContext,cx1,cy1,cx2,cy2,x,y);
}

// gets the last point of the current path, (0,0) if not yet set
void wxCairoPathData::GetCurrentPoint( wxDouble* x, wxDouble* y) const
{
    double dx,dy;
    cairo_get_current_point(m_pathContext,&dx,&dy);
    if (x)
        *x = dx;
    if (y)
        *y = dy;
}

void wxCairoPathData::AddArc( wxDouble x, wxDouble y, wxDouble r, double startAngle, double endAngle, bool clockwise )
{
    // as clockwise means positive in our system (y pointing downwards)
    // TODO make this interpretation dependent of the
    // real device trans
    if ( clockwise||(endAngle-startAngle)>=2*M_PI)
        cairo_arc(m_pathContext,x,y,r,startAngle,endAngle);
    else
        cairo_arc_negative(m_pathContext,x,y,r,startAngle,endAngle);
}

// transforms each point of this path by the matrix
void wxCairoPathData::Transform( const wxGraphicsMatrixData* matrix )
{
    // as we don't have a true path object, we have to apply the inverse
    // matrix to the context
    cairo_matrix_t m = *((cairo_matrix_t*) matrix->GetNativeMatrix());
    cairo_matrix_invert( &m );
    cairo_transform(m_pathContext,&m);
}

// gets the bounding box enclosing all points (possibly including control points)
void wxCairoPathData::GetBox(wxDouble *x, wxDouble *y, wxDouble *w, wxDouble *h) const
{
    double x1,y1,x2,y2;

    cairo_stroke_extents( m_pathContext, &x1, &y1, &x2, &y2 );
    if ( x2 < x1 )
    {
        *x = x2;
        *w = x1-x2;
    }
    else
    {
        *x = x1;
        *w = x2-x1;
    }

    if( y2 < y1 )
    {
        *y = y2;
        *h = y1-y2;
    }
    else
    {
        *y = y1;
        *h = y2-y1;
    }
}

bool wxCairoPathData::Contains( wxDouble x, wxDouble y, wxPolygonFillMode fillStyle ) const
{
    cairo_set_fill_rule(m_pathContext,fillStyle==wxODDEVEN_RULE ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    return cairo_in_fill( m_pathContext, x, y) != 0;
}

//-----------------------------------------------------------------------------
// wxCairoMatrixData implementation
//-----------------------------------------------------------------------------

wxCairoMatrixData::wxCairoMatrixData(wxGraphicsRenderer* renderer, const cairo_matrix_t* matrix )
    : wxGraphicsMatrixData(renderer)
{
    if ( matrix )
        m_matrix = *matrix;
}

wxCairoMatrixData::~wxCairoMatrixData()
{
    // nothing to do
}

wxGraphicsObjectRefData *wxCairoMatrixData::Clone() const
{
    return new wxCairoMatrixData(GetRenderer(),&m_matrix);
}

// concatenates the matrix
void wxCairoMatrixData::Concat( const wxGraphicsMatrixData *t )
{
    cairo_matrix_multiply( &m_matrix, &m_matrix, (cairo_matrix_t*) t->GetNativeMatrix());
}

// sets the matrix to the respective values
void wxCairoMatrixData::Set(wxDouble a, wxDouble b, wxDouble c, wxDouble d,
                        wxDouble tx, wxDouble ty)
{
    cairo_matrix_init( &m_matrix, a, b, c, d, tx, ty);
}

// gets the component valuess of the matrix
void wxCairoMatrixData::Get(wxDouble* a, wxDouble* b,  wxDouble* c,
                            wxDouble* d, wxDouble* tx, wxDouble* ty) const
{
    if (a)  *a = m_matrix.xx;
    if (b)  *b = m_matrix.yx;
    if (c)  *c = m_matrix.xy;
    if (d)  *d = m_matrix.yy;
    if (tx) *tx= m_matrix.x0;
    if (ty) *ty= m_matrix.y0;
}

// makes this the inverse matrix
void wxCairoMatrixData::Invert()
{
    cairo_matrix_invert( &m_matrix );
}

// returns true if the elements of the transformation matrix are equal ?
bool wxCairoMatrixData::IsEqual( const wxGraphicsMatrixData* t) const
{
    const cairo_matrix_t* tm = (cairo_matrix_t*) t->GetNativeMatrix();
    return (
        m_matrix.xx == tm->xx &&
        m_matrix.yx == tm->yx &&
        m_matrix.xy == tm->xy &&
        m_matrix.yy == tm->yy &&
        m_matrix.x0 == tm->x0 &&
        m_matrix.y0 == tm->y0 ) ;
}

// return true if this is the identity matrix
bool wxCairoMatrixData::IsIdentity() const
{
    return ( m_matrix.xx == 1 && m_matrix.yy == 1 &&
        m_matrix.yx == 0 && m_matrix.xy == 0 && m_matrix.x0 == 0 && m_matrix.y0 == 0);
}

//
// transformation
//

// add the translation to this matrix
void wxCairoMatrixData::Translate( wxDouble dx , wxDouble dy )
{
    cairo_matrix_translate( &m_matrix, dx, dy) ;
}

// add the scale to this matrix
void wxCairoMatrixData::Scale( wxDouble xScale , wxDouble yScale )
{
    cairo_matrix_scale( &m_matrix, xScale, yScale) ;
}

// add the rotation to this matrix (radians)
void wxCairoMatrixData::Rotate( wxDouble angle )
{
    cairo_matrix_rotate( &m_matrix, angle) ;
}

//
// apply the transforms
//

// applies that matrix to the point
void wxCairoMatrixData::TransformPoint( wxDouble *x, wxDouble *y ) const
{
    double lx = *x, ly = *y ;
    cairo_matrix_transform_point( &m_matrix, &lx, &ly);
    *x = lx;
    *y = ly;
}

// applies the matrix except for translations
void wxCairoMatrixData::TransformDistance( wxDouble *dx, wxDouble *dy ) const
{
    double lx = *dx, ly = *dy ;
    cairo_matrix_transform_distance( &m_matrix, &lx, &ly);
    *dx = lx;
    *dy = ly;
}

// returns the native representation
void * wxCairoMatrixData::GetNativeMatrix() const
{
    return (void*) &m_matrix;
}

// wxCairoBitmap implementation
//-----------------------------------------------------------------------------

wxCairoBitmapData::wxCairoBitmapData( wxGraphicsRenderer* renderer, cairo_surface_t* bitmap ) :
    wxGraphicsObjectRefData( renderer )
{
    m_surface = bitmap;
    m_pattern = cairo_pattern_create_for_surface(m_surface);
}

wxCairoBitmapData::wxCairoBitmapData( wxGraphicsRenderer* renderer, const wxBitmap& bmp ) : wxGraphicsObjectRefData( renderer )
{
    wxCHECK_RET( bmp.IsOk(), wxT("Invalid bitmap in wxCairoContext::DrawBitmap"));

    int bw = m_width = bmp.GetWidth();
    int bh = m_height = bmp.GetHeight();
    wxBitmap bmpSource = bmp;  // we need a non-const instance
    m_buffer = new unsigned char[bw*bh*4];
    wxUint32* data = (wxUint32*)m_buffer;

    // Create a surface object and copy the bitmap pixel data to it.  if the
    // image has alpha (or a mask represented as alpha) then we'll use a
    // different format and iterator than if it doesn't...
    if (bmpSource.HasAlpha() || bmpSource.GetMask())
    {
        m_surface = cairo_image_surface_create_for_data(
            m_buffer, CAIRO_FORMAT_ARGB32, bw, bh, bw*4);
        wxAlphaPixelData pixData(bmpSource, wxPoint(0,0), wxSize(bw, bh));
        wxCHECK_RET( pixData, wxT("Failed to gain raw access to bitmap data."));

        wxAlphaPixelData::Iterator p(pixData);
        for (int y=0; y<bh; y++)
        {
            wxAlphaPixelData::Iterator rowStart = p;
            for (int x=0; x<bw; x++)
            {
                // Each pixel in CAIRO_FORMAT_ARGB32 is a 32-bit quantity,
                // with alpha in the upper 8 bits, then red, then green, then
                // blue. The 32-bit quantities are stored native-endian.
                // Pre-multiplied alpha is used.
                unsigned char alpha = p.Alpha();
                if (alpha == 0)
                    *data = 0;
                else
                    *data = ( alpha                      << 24
                              | (p.Red() * alpha/255)    << 16
                              | (p.Green() * alpha/255)  <<  8
                              | (p.Blue() * alpha/255) );
                ++data;
                ++p;
            }
            p = rowStart;
            p.OffsetY(pixData, 1);
        }
    }
    else  // no alpha
    {
        m_surface = cairo_image_surface_create_for_data(
            m_buffer, CAIRO_FORMAT_RGB24, bw, bh, bw*4);
        wxNativePixelData pixData(bmpSource, wxPoint(0,0), wxSize(bw, bh));
        wxCHECK_RET( pixData, wxT("Failed to gain raw access to bitmap data."));

        wxNativePixelData::Iterator p(pixData);
        for (int y=0; y<bh; y++)
        {
            wxNativePixelData::Iterator rowStart = p;
            for (int x=0; x<bw; x++)
            {
                // Each pixel in CAIRO_FORMAT_RGB24 is a 32-bit quantity, with
                // the upper 8 bits unused. Red, Green, and Blue are stored in
                // the remaining 24 bits in that order.  The 32-bit quantities
                // are stored native-endian.
                *data = ( p.Red() << 16 | p.Green() << 8 | p.Blue() );
                ++data;
                ++p;
            }
            p = rowStart;
            p.OffsetY(pixData, 1);
        }
    }
    m_pattern = cairo_pattern_create_for_surface(m_surface);
}

wxCairoBitmapData::~wxCairoBitmapData()
{
    if (m_pattern)
        cairo_pattern_destroy(m_pattern);

    if (m_surface)
        cairo_surface_destroy(m_surface);

    delete [] m_buffer;
}



//-----------------------------------------------------------------------------
// wxCairoContext implementation
//-----------------------------------------------------------------------------

class wxCairoOffsetHelper
{
public :
    wxCairoOffsetHelper( cairo_t* ctx , bool offset )
    {
        m_ctx = ctx;
        m_offset = offset;
        if ( m_offset )
             cairo_translate( m_ctx, 0.5, 0.5 );
    }
    ~wxCairoOffsetHelper( )
    {
        if ( m_offset )
            cairo_translate( m_ctx, -0.5, -0.5 );
    }
public :
    cairo_t* m_ctx;
    bool m_offset;
} ;

wxCairoContext::wxCairoContext( wxGraphicsRenderer* renderer, const wxPrinterDC& dc )
: wxGraphicsContext(renderer)
{
#ifdef __WXGTK20__
    const wxDCImpl *impl = dc.GetImpl();
    Init( (cairo_t*) impl->GetCairoContext() );

    wxPoint org = dc.GetDeviceOrigin();
    cairo_translate( m_context, org.x, org.y );

    double sx,sy;
    dc.GetUserScale( &sx, &sy );
    cairo_scale( m_context, sx, sy );

    org = dc.GetLogicalOrigin();
    cairo_translate( m_context, -org.x, -org.y );
#endif
}

wxCairoContext::wxCairoContext( wxGraphicsRenderer* renderer, const wxWindowDC& dc )
: wxGraphicsContext(renderer)
{
#ifdef __WXGTK20__
    wxGTKDCImpl *impldc = (wxGTKDCImpl*) dc.GetImpl();
    Init( gdk_cairo_create( impldc->GetGDKWindow() ) );

#if 0
    wxGraphicsMatrix matrix = CreateMatrix();

    wxPoint org = dc.GetDeviceOrigin();
    matrix.Translate( org.x, org.y );

    org = dc.GetLogicalOrigin();
    matrix.Translate( -org.x, -org.y );

    double sx,sy;
    dc.GetUserScale( &sx, &sy );
    matrix.Scale( sx, sy );

    ConcatTransform( matrix );
#endif
#endif

#ifdef __WXMAC__
    int width, height;
    dc.GetSize( &width, &height );
    CGContextRef cgcontext = (CGContextRef)dc.GetWindow()->MacGetCGContextRef();
    cairo_surface_t* surface = cairo_quartz_surface_create_for_cg_context(cgcontext, width, height);
    Init( cairo_create( surface ) );
    cairo_surface_destroy( surface );
#endif
}

wxCairoContext::wxCairoContext( wxGraphicsRenderer* renderer, const wxMemoryDC& dc )
: wxGraphicsContext(renderer)
{
#ifdef __WXGTK20__
    wxGTKDCImpl *impldc = (wxGTKDCImpl*) dc.GetImpl();
    Init( gdk_cairo_create( impldc->GetGDKWindow() ) );

#if 0
    wxGraphicsMatrix matrix = CreateMatrix();

    wxPoint org = dc.GetDeviceOrigin();
    matrix.Translate( org.x, org.y );

    org = dc.GetLogicalOrigin();
    matrix.Translate( -org.x, -org.y );

    double sx,sy;
    dc.GetUserScale( &sx, &sy );
    matrix.Scale( sx, sy );

    ConcatTransform( matrix );
#endif
#endif

#ifdef __WXMAC__
    int width, height;
    dc.GetSize( &width, &height );
    CGContextRef cgcontext = (CGContextRef)dc.GetWindow()->MacGetCGContextRef();
    cairo_surface_t* surface = cairo_quartz_surface_create_for_cg_context(cgcontext, width, height);
    Init( cairo_create( surface ) );
    cairo_surface_destroy( surface );
#endif
}

#ifdef __WXGTK20__
wxCairoContext::wxCairoContext( wxGraphicsRenderer* renderer, GdkDrawable *drawable )
: wxGraphicsContext(renderer)
{
    Init( gdk_cairo_create( drawable ) );
}
#endif

#ifdef __WXMSW__
wxCairoContext::wxCairoContext( wxGraphicsRenderer* renderer, HDC handle )
: wxGraphicsContext(renderer)
{
    m_mswSurface = cairo_win32_surface_create(handle);
    m_context = cairo_create(m_mswSurface);
    PushState();
    PushState();
}
#endif


wxCairoContext::wxCairoContext( wxGraphicsRenderer* renderer, cairo_t *context )
: wxGraphicsContext(renderer)
{
    Init( context );
}

wxCairoContext::wxCairoContext( wxGraphicsRenderer* renderer, wxWindow *window)
: wxGraphicsContext(renderer)
{
#ifdef __WXGTK__
    // something along these lines (copied from dcclient)

    // Some controls don't have m_wxwindow - like wxStaticBox, but the user
    // code should still be able to create wxClientDCs for them, so we will
    // use the parent window here then.
    if (window->m_wxwindow == NULL)
    {
        window = window->GetParent();
    }

    wxASSERT_MSG( window->m_wxwindow, wxT("wxCairoContext needs a widget") );

    Init(gdk_cairo_create(window->GTKGetDrawingWindow()));
#endif
}

wxCairoContext::~wxCairoContext()
{
    if ( m_context )
    {
        PopState();
#ifdef __WXMSW__
    m_mswSurface = cairo_win32_surface_create((HDC)window->GetHandle());
    m_context = cairo_create(m_mswSurface);
#endif
        PopState();
        cairo_destroy(m_context);
    }
#ifdef __WXMSW__
    if ( m_mswSurface )
        cairo_surface_destroy(m_mswSurface);
#endif
}

void wxCairoContext::Init(cairo_t *context)
{
    m_context = context ;
    PushState();
    PushState();
}


void wxCairoContext::Clip( const wxRegion& region )
{
    // Create a path with all the rectangles in the region
    wxGraphicsPath path = GetRenderer()->CreatePath();
    wxRegionIterator ri(region);
    while (ri)
    {
        path.AddRectangle(ri.GetX(), ri.GetY(), ri.GetW(), ri.GetH());
        ++ri;
    }

    // Put it in the context
    cairo_path_t* cp = (cairo_path_t*) path.GetNativePath() ;
    cairo_append_path(m_context, cp);

    // clip to that path
    cairo_clip(m_context);
    path.UnGetNativePath(cp);
}

void wxCairoContext::Clip( wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    // Create a path with this rectangle
    wxGraphicsPath path = GetRenderer()->CreatePath();
    path.AddRectangle(x,y,w,h);

    // Put it in the context
    cairo_path_t* cp = (cairo_path_t*) path.GetNativePath() ;
    cairo_append_path(m_context, cp);

    // clip to that path
    cairo_clip(m_context);
    path.UnGetNativePath(cp);
}

void wxCairoContext::ResetClip()
{
    cairo_reset_clip(m_context);
}


void wxCairoContext::StrokePath( const wxGraphicsPath& path )
{
    if ( !m_pen.IsNull() )
    {
        wxCairoOffsetHelper helper( m_context, ShouldOffset() ) ;
        cairo_path_t* cp = (cairo_path_t*) path.GetNativePath() ;
        cairo_append_path(m_context,cp);
        ((wxCairoPenData*)m_pen.GetRefData())->Apply(this);
        cairo_stroke(m_context);
        path.UnGetNativePath(cp);
    }
}

void wxCairoContext::FillPath( const wxGraphicsPath& path , wxPolygonFillMode fillStyle )
{
    if ( !m_brush.IsNull() )
    {
        wxCairoOffsetHelper helper( m_context, ShouldOffset() ) ;
        cairo_path_t* cp = (cairo_path_t*) path.GetNativePath() ;
        cairo_append_path(m_context,cp);
        ((wxCairoBrushData*)m_brush.GetRefData())->Apply(this);
        cairo_set_fill_rule(m_context,fillStyle==wxODDEVEN_RULE ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
        cairo_fill(m_context);
        path.UnGetNativePath(cp);
    }
}

void wxCairoContext::Rotate( wxDouble angle )
{
    cairo_rotate(m_context,angle);
}

void wxCairoContext::Translate( wxDouble dx , wxDouble dy )
{
    cairo_translate(m_context,dx,dy);
}

void wxCairoContext::Scale( wxDouble xScale , wxDouble yScale )
{
    cairo_scale(m_context,xScale,yScale);
}

// concatenates this transform with the current transform of this context
void wxCairoContext::ConcatTransform( const wxGraphicsMatrix& matrix )
{
    cairo_transform(m_context,(const cairo_matrix_t *) matrix.GetNativeMatrix());
}

// sets the transform of this context
void wxCairoContext::SetTransform( const wxGraphicsMatrix& matrix )
{
    cairo_set_matrix(m_context,(const cairo_matrix_t*) matrix.GetNativeMatrix());
}

// gets the matrix of this context
wxGraphicsMatrix wxCairoContext::GetTransform() const
{
    wxGraphicsMatrix matrix = CreateMatrix();
    cairo_get_matrix(m_context,(cairo_matrix_t*) matrix.GetNativeMatrix());
    return matrix;
}



void wxCairoContext::PushState()
{
    cairo_save(m_context);
}

void wxCairoContext::PopState()
{
    cairo_restore(m_context);
}

void wxCairoContext::DrawBitmap( const wxBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    wxGraphicsBitmap bitmap = GetRenderer()->CreateBitmap(bmp);
    DrawBitmap(bitmap, x, y, w, h);

}

void wxCairoContext::DrawBitmap(const wxGraphicsBitmap &bmp, wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    PushState();

    // In case we're scaling the image by using a width and height different
    // than the bitmap's size create a pattern transformation on the surface and
    // draw the transformed pattern.
    wxCairoBitmapData* data = static_cast<wxCairoBitmapData*>(bmp.GetRefData());
    cairo_pattern_t* pattern = data->GetCairoPattern();
    wxSize size = data->GetSize();

    wxDouble scaleX = w / size.GetWidth();
    wxDouble scaleY = h / size.GetHeight();

    // prepare to draw the image
    cairo_translate(m_context, x, y);
    cairo_scale(m_context, scaleX, scaleY);
    cairo_set_source(m_context, pattern);
    // use the original size here since the context is scaled already...
    cairo_rectangle(m_context, 0, 0, size.GetWidth(), size.GetHeight());
    // fill the rectangle using the pattern
    cairo_fill(m_context);

    PopState();
}

void wxCairoContext::DrawIcon( const wxIcon &icon, wxDouble x, wxDouble y, wxDouble w, wxDouble h )
{
    // An icon is a bitmap on wxGTK, so do this the easy way.  When we want to
    // start using the Cairo backend on other platforms then we may need to
    // fiddle with this...
    DrawBitmap(icon, x, y, w, h);
}


void wxCairoContext::DoDrawText(const wxString& str, wxDouble x, wxDouble y)
{
    wxCHECK_RET( !m_font.IsNull(),
                 wxT("wxCairoContext::DrawText - no valid font set") );

    if ( str.empty())
        return;

    const wxCharBuffer data = str.utf8_str();
    if ( !data )
        return;

    ((wxCairoFontData*)m_font.GetRefData())->Apply(this);

#ifdef __WXGTK__
    size_t datalen = strlen(data);

    PangoLayout *layout = pango_cairo_create_layout (m_context);
    wxCairoFontData* font_data = (wxCairoFontData*) m_font.GetRefData();
    pango_layout_set_font_description( layout, font_data->GetFont());
    pango_layout_set_text(layout, data, datalen);

    if (font_data->GetUnderlined())
    {
        PangoAttrList *attrs = pango_attr_list_new();
        PangoAttribute *attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
        pango_attr_list_insert(attrs, attr);
        pango_layout_set_attributes(layout, attrs);
        pango_attr_list_unref(attrs);
    }

    cairo_move_to(m_context, x, y);
    pango_cairo_show_layout (m_context, layout);

    g_object_unref (layout);
#else
    // Cairo's x,y for drawing text is at the baseline, so we need to adjust
    // the position we move to by the ascent.
    cairo_font_extents_t fe;
    cairo_font_extents(m_context, &fe);
    cairo_move_to(m_context, x, y+fe.ascent);

    cairo_show_text(m_context, data);
#endif
}

void wxCairoContext::GetTextExtent( const wxString &str, wxDouble *width, wxDouble *height,
                                    wxDouble *descent, wxDouble *externalLeading ) const
{
    wxCHECK_RET( !m_font.IsNull(), wxT("wxCairoContext::GetTextExtent - no valid font set") );

    if ( width )
        *width = 0;
    if ( height )
        *height = 0;
    if ( descent )
        *descent = 0;
    if ( externalLeading )
        *externalLeading = 0;

    if ( str.empty())
        return;

#ifdef __WXGTK__
    int w, h;

    PangoLayout *layout = pango_cairo_create_layout (m_context);
    pango_layout_set_font_description( layout, ((wxCairoFontData*)m_font.GetRefData())->GetFont());
    const wxCharBuffer data = str.utf8_str();
    if ( !data )
    {
        return;
    }
    pango_layout_set_text( layout, data, strlen(data) );
    pango_layout_get_pixel_size (layout, &w, &h);
    if ( width )
        *width = w;
    if ( height )
        *height = h;
    if (descent)
    {
        PangoLayoutIter *iter = pango_layout_get_iter(layout);
        int baseline = pango_layout_iter_get_baseline(iter);
        pango_layout_iter_free(iter);
        *descent = h - PANGO_PIXELS(baseline);
    }
    g_object_unref (layout);
#else
    ((wxCairoFontData*)m_font.GetRefData())->Apply((wxCairoContext*)this);

    if (width)
    {
        const wxWX2MBbuf buf(str.mb_str(wxConvUTF8));
        cairo_text_extents_t te;
        cairo_text_extents(m_context, buf, &te);
        *width = te.width;
    }

    if (height || descent || externalLeading)
    {
        cairo_font_extents_t fe;
        cairo_font_extents(m_context, &fe);

        // some backends have negative descents

        if ( fe.descent < 0 )
            fe.descent = -fe.descent;

        if ( fe.height < (fe.ascent + fe.descent ) )
        {
            // some backends are broken re height ... (eg currently ATSUI)
            fe.height = fe.ascent + fe.descent;
        }

        if (height)
            *height = fe.height;
        if ( descent )
            *descent = fe.descent;
        if ( externalLeading )
            *externalLeading = wxMax(0, fe.height - (fe.ascent + fe.descent));
    }
#endif
}

void wxCairoContext::GetPartialTextExtents(const wxString& text, wxArrayDouble& widths) const
{
    widths.Empty();
    widths.Add(0, text.length());

    wxCHECK_RET( !m_font.IsNull(), wxT("wxCairoContext::GetPartialTextExtents - no valid font set") );

    if (text.empty())
        return;

    // TODO
}

void * wxCairoContext::GetNativeContext()
{
    return m_context;
}

bool wxCairoContext::SetAntialiasMode(wxAntialiasMode antialias)
{
    if (m_antialias == antialias)
        return true;

    m_antialias = antialias;

    cairo_antialias_t antialiasMode;
    switch (antialias)
    {
        case wxANTIALIAS_DEFAULT:
            antialiasMode = CAIRO_ANTIALIAS_DEFAULT;
            break;
        case wxANTIALIAS_NONE:
            antialiasMode = CAIRO_ANTIALIAS_NONE;
            break;
        default:
            return false;
    }
    cairo_set_antialias(m_context, antialiasMode);
    return true;
}

bool wxCairoContext::SetCompositionMode(wxCompositionMode op)
{
    if ( m_composition == op )
        return true;

    m_composition = op;
    cairo_operator_t cop;
    switch (op)
    {
        case wxCOMPOSITION_CLEAR:
            cop = CAIRO_OPERATOR_CLEAR;
            break;
        case wxCOMPOSITION_SOURCE:
            cop = CAIRO_OPERATOR_SOURCE;
            break;
        case wxCOMPOSITION_OVER:
            cop = CAIRO_OPERATOR_OVER;
            break;
        case wxCOMPOSITION_IN:
            cop = CAIRO_OPERATOR_IN;
            break;
        case wxCOMPOSITION_OUT:
            cop = CAIRO_OPERATOR_OUT;
            break;
        case wxCOMPOSITION_ATOP:
            cop = CAIRO_OPERATOR_ATOP;
            break;
        case wxCOMPOSITION_DEST:
            cop = CAIRO_OPERATOR_DEST;
            break;
        case wxCOMPOSITION_DEST_OVER:
            cop = CAIRO_OPERATOR_DEST_OVER;
            break;
        case wxCOMPOSITION_DEST_IN:
            cop = CAIRO_OPERATOR_DEST_IN;
            break;
        case wxCOMPOSITION_DEST_OUT:
            cop = CAIRO_OPERATOR_DEST_OUT;
            break;
        case wxCOMPOSITION_DEST_ATOP:
            cop = CAIRO_OPERATOR_DEST_ATOP;
            break;
        case wxCOMPOSITION_XOR:
            cop = CAIRO_OPERATOR_XOR;
            break;
        case wxCOMPOSITION_ADD:
            cop = CAIRO_OPERATOR_ADD;
            break;
        default:
            return false;
    }
    cairo_set_operator(m_context, cop);
    return true;
}

void wxCairoContext::BeginLayer(wxDouble opacity)
{
    m_layerOpacities.push_back(opacity);
    cairo_push_group(m_context);
}

void wxCairoContext::EndLayer()
{
    float opacity = m_layerOpacities.back();
    m_layerOpacities.pop_back();
    cairo_pop_group_to_source(m_context);
    cairo_paint_with_alpha(m_context,opacity);
}

//-----------------------------------------------------------------------------
// wxCairoRenderer declaration
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxCairoRenderer : public wxGraphicsRenderer
{
public :
    wxCairoRenderer() {}

    virtual ~wxCairoRenderer() {}

    // Context

    virtual wxGraphicsContext * CreateContext( const wxWindowDC& dc);
    virtual wxGraphicsContext * CreateContext( const wxMemoryDC& dc);
    virtual wxGraphicsContext * CreateContext( const wxPrinterDC& dc);

    virtual wxGraphicsContext * CreateContextFromNativeContext( void * context );

    virtual wxGraphicsContext * CreateContextFromNativeWindow( void * window );

    virtual wxGraphicsContext * CreateContext( wxWindow* window );

    virtual wxGraphicsContext * CreateMeasuringContext();

    // Path

    virtual wxGraphicsPath CreatePath();

    // Matrix

    virtual wxGraphicsMatrix CreateMatrix( wxDouble a=1.0, wxDouble b=0.0, wxDouble c=0.0, wxDouble d=1.0,
        wxDouble tx=0.0, wxDouble ty=0.0);


    virtual wxGraphicsPen CreatePen(const wxPen& pen) ;

    virtual wxGraphicsBrush CreateBrush(const wxBrush& brush ) ;

    virtual wxGraphicsBrush
    CreateLinearGradientBrush(wxDouble x1, wxDouble y1,
                              wxDouble x2, wxDouble y2,
                              const wxGraphicsGradientStops& stops);

    virtual wxGraphicsBrush
    CreateRadialGradientBrush(wxDouble xo, wxDouble yo,
                              wxDouble xc, wxDouble yc,
                              wxDouble radius,
                              const wxGraphicsGradientStops& stops);

    // sets the font
    virtual wxGraphicsFont CreateFont( const wxFont &font , const wxColour &col = *wxBLACK ) ;

    // create a native bitmap representation
    virtual wxGraphicsBitmap CreateBitmap( const wxBitmap &bitmap );

    // create a graphics bitmap from a native bitmap
    virtual wxGraphicsBitmap CreateBitmapFromNativeBitmap( void* bitmap );

    // create a subimage from a native image representation
    virtual wxGraphicsBitmap CreateSubBitmap( const wxGraphicsBitmap &bitmap, wxDouble x, wxDouble y, wxDouble w, wxDouble h  );

private :
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxCairoRenderer)
} ;

//-----------------------------------------------------------------------------
// wxCairoRenderer implementation
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxCairoRenderer,wxGraphicsRenderer)

static wxCairoRenderer gs_cairoGraphicsRenderer;
// temporary hack to allow creating a cairo context on any platform
extern wxGraphicsRenderer* gCairoRenderer;
wxGraphicsRenderer* gCairoRenderer = &gs_cairoGraphicsRenderer;

wxGraphicsContext * wxCairoRenderer::CreateContext( const wxWindowDC& dc)
{
    return new wxCairoContext(this,dc);
}

wxGraphicsContext * wxCairoRenderer::CreateContext( const wxMemoryDC& dc)
{
    return new wxCairoContext(this,dc);
}

wxGraphicsContext * wxCairoRenderer::CreateContext( const wxPrinterDC& dc)
{
#ifdef __WXGTK20__
    const wxDCImpl *impl = dc.GetImpl();
    cairo_t* context = (cairo_t*) impl->GetCairoContext();
    if (context)
       return new wxCairoContext(this,dc);
    else
#endif
       return NULL;
}

wxGraphicsContext * wxCairoRenderer::CreateContextFromNativeContext( void * context )
{
#ifdef __WXMSW__
    return new wxCairoContext(this,(HDC)context);
#endif
#ifdef __WXGTK__
    return new wxCairoContext(this,(cairo_t*)context);
#endif
}


wxGraphicsContext * wxCairoRenderer::CreateContextFromNativeWindow( void * window )
{
#ifdef __WXGTK__
    return new wxCairoContext(this,(GdkDrawable*)window);
#else
    return NULL;
#endif
}

wxGraphicsContext * wxCairoRenderer::CreateMeasuringContext()
{
#ifdef __WXGTK__
    return CreateContextFromNativeWindow(gdk_get_default_root_window());
#endif
    return NULL;
    // TODO
}

wxGraphicsContext * wxCairoRenderer::CreateContext( wxWindow* window )
{
    return new wxCairoContext(this, window );
}

// Path

wxGraphicsPath wxCairoRenderer::CreatePath()
{
    wxGraphicsPath path;
    path.SetRefData( new wxCairoPathData(this) );
    return path;
}


// Matrix

wxGraphicsMatrix wxCairoRenderer::CreateMatrix( wxDouble a, wxDouble b, wxDouble c, wxDouble d,
                                                wxDouble tx, wxDouble ty)

{
    wxGraphicsMatrix m;
    wxCairoMatrixData* data = new wxCairoMatrixData( this );
    data->Set( a,b,c,d,tx,ty ) ;
    m.SetRefData(data);
    return m;
}

wxGraphicsPen wxCairoRenderer::CreatePen(const wxPen& pen)
{
    if ( !pen.Ok() || pen.GetStyle() == wxPENSTYLE_TRANSPARENT )
        return wxNullGraphicsPen;
    else
    {
        wxGraphicsPen p;
        p.SetRefData(new wxCairoPenData( this, pen ));
        return p;
    }
}

wxGraphicsBrush wxCairoRenderer::CreateBrush(const wxBrush& brush )
{
    if ( !brush.Ok() || brush.GetStyle() == wxBRUSHSTYLE_TRANSPARENT )
        return wxNullGraphicsBrush;
    else
    {
        wxGraphicsBrush p;
        p.SetRefData(new wxCairoBrushData( this, brush ));
        return p;
    }
}

wxGraphicsBrush
wxCairoRenderer::CreateLinearGradientBrush(wxDouble x1, wxDouble y1,
                                           wxDouble x2, wxDouble y2,
                                           const wxGraphicsGradientStops& stops)
{
    wxGraphicsBrush p;
    wxCairoBrushData* d = new wxCairoBrushData( this );
    d->CreateLinearGradientBrush(x1, y1, x2, y2, stops);
    p.SetRefData(d);
    return p;
}

wxGraphicsBrush
wxCairoRenderer::CreateRadialGradientBrush(wxDouble xo, wxDouble yo,
                                           wxDouble xc, wxDouble yc, wxDouble r,
                                           const wxGraphicsGradientStops& stops)
{
    wxGraphicsBrush p;
    wxCairoBrushData* d = new wxCairoBrushData( this );
    d->CreateRadialGradientBrush(xo, yo, xc, yc, r, stops);
    p.SetRefData(d);
    return p;
}

// sets the font
wxGraphicsFont wxCairoRenderer::CreateFont( const wxFont &font , const wxColour &col )
{
    if ( font.Ok() )
    {
        wxGraphicsFont p;
        p.SetRefData(new wxCairoFontData( this , font, col ));
        return p;
    }
    else
        return wxNullGraphicsFont;
}

wxGraphicsBitmap wxCairoRenderer::CreateBitmap( const wxBitmap& bmp )
{
    if ( bmp.Ok() )
    {
        wxGraphicsBitmap p;
        p.SetRefData(new wxCairoBitmapData( this , bmp ));
        return p;
    }
    else
        return wxNullGraphicsBitmap;
}

wxGraphicsBitmap wxCairoRenderer::CreateBitmapFromNativeBitmap( void* bitmap )
{
    if ( bitmap != NULL )
    {
        wxGraphicsBitmap p;
        p.SetRefData(new wxCairoBitmapData( this , (cairo_surface_t*) bitmap ));
        return p;
    }
    else
        return wxNullGraphicsBitmap;
}

wxGraphicsBitmap
wxCairoRenderer::CreateSubBitmap(const wxGraphicsBitmap& WXUNUSED(bitmap),
                                 wxDouble WXUNUSED(x),
                                 wxDouble WXUNUSED(y),
                                 wxDouble WXUNUSED(w),
                                 wxDouble WXUNUSED(h))
{
    wxFAIL_MSG("wxCairoRenderer::CreateSubBitmap is not implemented.");
    return wxNullGraphicsBitmap;
}

wxGraphicsRenderer* wxGraphicsRenderer::GetCairoRenderer()
{
    return &gs_cairoGraphicsRenderer;
}

#else // !wxUSE_CAIRO

wxGraphicsRenderer* wxGraphicsRenderer::GetCairoRenderer()
{
    return NULL;
}

#endif  // wxUSE_CAIRO/!wxUSE_CAIRO

// MSW and OS X have their own native default renderers, but the other ports
// use Cairo by default
#if !(defined(__WXMSW__) || defined(__WXOSX__))
wxGraphicsRenderer* wxGraphicsRenderer::GetDefaultRenderer()
{
    return GetCairoRenderer();
}
#endif // !(__WXMSW__ || __WXOSX__)

#endif // wxUSE_GRAPHICS_CONTEXT
