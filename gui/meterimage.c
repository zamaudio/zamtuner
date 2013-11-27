
#include "img/meter.c"

static const float c_wsh[4] = {1.0, 1.0, 1.0, 0.5};

#define img_draw_needle(CR, VAL, R1, R2, COL, LW) \
	img_draw_needle_x(CR, VAL, _xc, _yc, R1, R2, COL, (LW) * _sc)

#define img_needle_label(CR, TXT, VAL, R1) \
	img_needle_label_col_x(CR, TXT, img_fontname, VAL, _xc, _yc, R1, c_wht)

#define img_needle_label_col(CR, TXT, VAL, R2, COL) \
	img_needle_label_col_x(CR, TXT, img_fontname, VAL, _xc, _yc, R2, COL)

#define MAKELOCALVARS(SCALE) \
	const float _sc = (float)(SCALE); \
	const float _rn = 150 * _sc; \
	const float _ri = 160 * _sc; \
	const float _rl = 180 * _sc; \
	const float _rs = 170 * _sc; \
	const float _xc = 149.5 * _sc; \
	const float _yc = 209.5 * _sc; \
	char img_fontname[48]; \
	char img_fonthuge[48]; \
	if (_sc <=1.0) { \
		sprintf(img_fontname, "Sans Bold 9"); \
		sprintf(img_fonthuge, "Sans Bold 12"); \
	} else { \
		sprintf(img_fontname, "Sans Bold %d", (int)rint(_rl/21)); \
		sprintf(img_fonthuge, "Sans Bold %d", (int)rint(_rn/10)); \
	}

static float img_radi (float v) {
	return (-M_PI/2 - M_PI/4) + v * M_PI/2;
}

static float img_deflect_vu(float db) {
	float v = pow(10, .05 *(db-18));
	return 5.6234149f * v;
}

static void img_draw_needle_x (
		cairo_t* cr, float val,
		const float _xc, const float _yc,
		const float _r1, const float _r2,
		const float * const col, const float lw)
{
	float  c, s;
	if (val < 0.00f) val = 0.00f;
	if (val > 1.05f) val = 1.05f;
	val = (val - 0.5f) * 1.5708f;
	c = cosf (val);
	s = sinf (val);
	cairo_new_path (cr);
	cairo_move_to (cr, _xc + s * _r1, _yc - c * _r1);
	cairo_line_to (cr, _xc + s * _r2, _yc - c * _r2);
	CairoSetSouerceRGBA(col);
	cairo_set_line_width (cr, lw);
	cairo_stroke (cr);

}

static void img_write_text(cairo_t* cr,
		const char *txt, const char *font,
		float x, float y, float ang)
{
	int tw, th;
	cairo_save(cr);
	PangoLayout * pl = pango_cairo_create_layout(cr);
	PangoFontDescription *desc = pango_font_description_from_string(font);
	pango_layout_set_font_description(pl, desc);
	pango_font_description_free(desc);
	pango_layout_set_text(pl, txt, -1);
	pango_layout_get_pixel_size(pl, &tw, &th);
	cairo_translate (cr, x, y);
	cairo_rotate (cr, ang);
	cairo_translate (cr, -tw/2.0, -th/2.0);
	pango_cairo_layout_path(cr, pl);
	pango_cairo_show_layout(cr, pl);
	g_object_unref(pl);
	cairo_restore(cr);
	cairo_new_path (cr);
}

void img_needle_label_col_x(cairo_t* cr,
		const char *txt, const char *font,
		float val,
		const float _xc, const float _yc,
		const float _r2, const float * const col)
{
	float  c, s;
	if (val < 0.00f) val = 0.00f;
	if (val > 1.05f) val = 1.05f;
	val = (val - 0.5f) * 1.5708f;
	c = cosf (val);
	s = sinf (val);

	CairoSetSouerceRGBA(col);
	img_write_text(cr, txt, font, _xc + s * _r2, _yc - c * _r2,  val);
}

static void img_draw_vu(cairo_t* cr, float scale) {
	/*MAKELOCALVARS(scale);
	const float _rx = 155 * _sc;
	const float	_yl = 95 * _sc;

	sprintf(img_fontname, "Sans %d", (int)rint(_rn / 21)); // XXX
	sprintf(img_fontname, "Sans Bold %d", (int)rint(_rl/20));

	CairoSetSouerceRGBA(c_blk);
*/
}


static cairo_surface_t* render_front_face(enum MtrType t, int w, int h) {
	cairo_surface_t *bg;
	unsigned char * img_tmp;
	img2surf((struct MyGimpImage const *) &img_meter, &bg, &img_tmp);
	cairo_surface_t *sf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
	cairo_t* cr = cairo_create(sf);

	cairo_save(cr);
	cairo_scale(cr, (float)w/300.0, (float)h/170.0);
	cairo_set_source_surface(cr, bg, 0, 0);
	cairo_rectangle (cr, 0, 0, 300, 170);
	cairo_fill(cr);
	cairo_restore(cr);

	const float _sc = (float)w/300.0;

	cairo_rectangle (cr, 7 * _sc, 7 * _sc, 284 * _sc, 128 * _sc);
	cairo_clip (cr);

	img_draw_vu(cr, _sc);
	cairo_destroy(cr);
	free(img_tmp);
	return sf;
}
