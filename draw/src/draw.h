#pragma once
#define MODULE_NAME "Draw"
/* sys */ 
/* app */
#include <gxinterface/0.0.1/gxmodule.h>
/** MYDIRECTORY CONSTANTS **/
#define UI MOD_UI_DIRECTORY"draw-0.0.2.ui"
//#define UI MOD_UI_DIRECTORY"dx7interface-0.0.1-gtk4_git-simplify.ui"
#define CSSFILE MOD_UI_DIRECTORY"theme.css"

extern "C" {
    std::tuple<std::shared_ptr<void>, Gtk::Box*, Glib::ustring> LoadPlug(uint8_t);
};

class Draw : public Gx_module {
    private:
        typedef struct st_99{
            gint8 val;
        }St_99;
        St_99 eg_rt[4];//  op eg rate
        St_99 eg_lvl[4];//  op eg level
        Glib::ustring mod_name;
        void block_all();
        void unblock_all();
        /* DRAWING */
        /* lines/curves */
        double line_width = 3.0;                                      // epaisseur
        /* point */
        double r_point = 4.5;
        double r_point_shadow = 9.0;
        std::array<double, 4> line_color = {0.4,0.8,0.6,1.0};       // couleur des courbes ( vert Dx7 ) format rgbax
        /* dashes */
        double dash_width = 0.5;                                      // epaisseur
        std::array<double, 4> dash_color = {0.4,0.8,0.6,0.8};    // couleur format rgba
        const std::vector< double > dash_pattern = {3.0, 5.0, 3.0, 5.0};  // pattern des pointillé ( lgt couleur, lgt espace , lgt couleur , lgt espace)
        double dash_offset = 0.0;                                   // offset du dash pattern
        /* grid */
        double grid_step_y=5.0, grid_step_x=20.0;                  // nombre de pas voulu en x y
        /* text */
        std::array<double, 4> text_color = {1.0, 0.5, 0.2, 0.8};
        std::array<double, 4> bg_color = {0.0, 0.0, 0.0, 0.0};
        /* Cairomm context helpers */
        int* get_cr_visible_size(const Cairo::RefPtr<Cairo::Context>&);  /* retourne la taille de sla zone visible */
        /* ADSR */
        std::pair<double,double> points[5];
        int p_drag = -1;
        void redraw_all_curve();
        void draw_background(const Cairo::RefPtr<Cairo::Context>&);                                 /* dessine le fond */
        void draw_grid(const Cairo::RefPtr<Cairo::Context>&, double, double);                       /* dessisne la grille */
        void draw_adsr(const Cairo::RefPtr<Cairo::Context>&, double, double, Glib::ustring);        /* dessine la courbe */
        void draw_point(const Cairo::RefPtr<Cairo::Context>&, double, double,double,bool);          /* dessine un point */
        void draw_note_off(const Cairo::RefPtr<Cairo::Context>&, double, double);
        void onresize(int, int);

        Glib::RefPtr<Gtk::GestureClick> controller_mouse_button;
        void mouse_click(int, double, double, Glib::ustring);
        void mouse_release(int, double, double, Glib::ustring);
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove;
        void mouse_moove(double, double, Glib::ustring);

    protected:
        void attach_signals();
        void dettach_signals();
        /* EVENTS */
        bool on_timeout();
        void on_draw_op_event(const Cairo::RefPtr<Cairo::Context>&, int, int, Glib::ustring);

        void on_eg_rt1_op1_event();
        void on_eg_rt2_op1_event();
        void on_eg_rt3_op1_event();
        void on_eg_rt4_op1_event();
        void on_eg_lvl1_op1_event();
        void on_eg_lvl2_op1_event();
        void on_eg_lvl3_op1_event();
        void on_eg_lvl4_op1_event();
        sigc::connection slot_eg_rt1_op1;
        sigc::connection slot_eg_rt2_op1;
        sigc::connection slot_eg_rt3_op1;
        sigc::connection slot_eg_rt4_op1;
        sigc::connection slot_eg_lvl1_op1;
        sigc::connection slot_eg_lvl2_op1;
        sigc::connection slot_eg_lvl3_op1;
        sigc::connection slot_eg_lvl4_op1;
    public:
        Draw(Glib::ustring,uint8_t);
		~Draw();
};
