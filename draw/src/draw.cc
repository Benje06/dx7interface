#include "draw.h"


extern "C" {
    std::tuple<std::shared_ptr<void>, Gtk::Box*, Glib::ustring> LoadPlug(uint8_t index){
        auto app = std::make_shared<Draw>(UI,index);
        Gtk::Box* mbox =  app->get_rootbox();
        return std::make_tuple(app, mbox, CSSFILE);
    };
}

Draw::Draw(Glib::ustring ui, uint8_t index) : Gx_module(ui,MODULE_NAME) {
    /*basic constructor */
    LOG_IN();
    #ifdef ENABLE_NLS
        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, PROGRAMNAME_LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
    #endif
    /* I/O init */
    Gio::init();

    controller_mouse_moove = Gtk::EventControllerMotion::create();
    controller_mouse_button = Gtk::GestureClick::create();
    controller_mouse_button->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller_mouse_button->set_button(1); // bouton gauche souris

    attach_signals();
    eg_rt[0].val=(get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->get_value();
    eg_rt[1].val=(get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->get_value();
    eg_rt[2].val=(get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->get_value();
    eg_rt[3].val=(get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->get_value();
    eg_lvl[0].val=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->get_value();
    eg_lvl[1].val=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->get_value();
    eg_lvl[2].val=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->get_value();
    eg_lvl[3].val=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->get_value();

    LOG_OUT();
};

Draw::~Draw(){ 
    LOG_IN();
    LOG_OUT();
};

void Draw::onresize(int width, int height){
    std::cout << "resize Width: " << width  << std::endl;
    std::cout << "resize Height: " << height << std::endl;
}

void Draw::mouse_moove(double x, double y, Glib::ustring name){
    if(p_drag != -1){
        double width = (double)get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1")->get_width();
        double height = get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1")->get_height();
        double x_ratio=( (width ) /400.0);
        double y_ratio=( (height) /99.0);
        int val_x = 0;
        int val_y = int( (height - y) - r_point ) / y_ratio;

        if(p_drag == 0){
            (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->set_value(val_y);
            return;
        }else{
            if (p_drag > 0 && p_drag < 4){
                val_x  = (x - points[p_drag-1].first) / x_ratio;
            }else{
                double x_noteoff=(width)*3.0/4.0;
                val_x = (x - x_noteoff) / x_ratio;
            };
        };
        val_x = std::max(0, (100 - int(val_x) ) );
        (get_gwidget<Gtk::SpinButton>("eg_rt"+tostr<int>(p_drag)+"_op1"))->set_value(val_x);
        (get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<int>(p_drag)+"_op1"))->set_value(val_y);
    };
};

void Draw::mouse_click(int n_press, double x, double y, Glib::ustring name){
    double width = (double)get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1")->get_width();
    double height = (double)get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1")->get_height();
    y = std::abs(height - y);
    std::cout << "Click souris: " << n_press << std::endl;
    std::cout << x <<" x "<< y << std::endl;
    std::cout << width <<" x "<< height << std::endl;
    for( int i = 0; i <= 4 ;i++){
        if(std::hypot(abs(points[i].first-x), abs(points[i].second-y)) <= r_point_shadow +1 ){
            std::cout << "on point: " << i << std::endl;
            p_drag=i;
            break;
        };
    };
};
void Draw::mouse_release(int n_press, double x, double y, Glib::ustring name){
    std::cout << "relaché souris: " << n_press << std::endl;
    std::cout << x <<" x "<< y << std::endl;
    std::cout << name << std::endl;
    p_drag=-1;
};

void Draw::draw_background(const Cairo::RefPtr<Cairo::Context>& cr){
    LOG_IN();
    cr->save();
    cr->set_source_rgba(bg_color[0], bg_color[1], bg_color[2], bg_color[3]);
    cr->paint();	// fill with color
    cr->restore();
    LOG_OUT();
};

void Draw::draw_grid(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height){
    LOG_IN();
    //LOG_IN();
    double x=0.0, y=0.0;                    // coordonee du point
    double x_step=(width/grid_step_x);      // representation d'un pas
    double y_step=(height/grid_step_y);
    cr->save();
    cr->set_source_rgba(dash_color[0], dash_color[1], dash_color[2], dash_color[3]);
    cr->set_dash(dash_pattern,dash_offset);
    cr->set_line_width(dash_width);
    y=0.0;
    for(x=0.0;x<=(width - dash_width); x+=x_step) { //draw verticals lines
        cr->move_to(x, y);
        cr->line_to(x, height);
    };
    cr->move_to( (width - dash_width), y);
    cr->line_to( (width - dash_width), height);
    x=0.0;
    for(y=0.0;y<=(height - dash_width); y+=y_step) {  //draw horizontals lines
        cr->move_to(x, y);
        cr->line_to(width, y);
    };
    cr->move_to(x , (height - dash_width) );
    cr->line_to(width, (height - dash_width));
    cr->stroke();  // trace le contour en preservant ceux deja tracés
    cr->restore();
    LOG_OUT();
};

void Draw::draw_point(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y,double width,bool red){
    // TODO set value in a var to be changed by interface
    //LOG_IN();
    double r,g,b,a;
    // Orange dx7 0.88,0.35,0.27,1.0
    r=line_color[0];
    g=line_color[1];
    b=line_color[2];

    //int width = (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->get_width();
    if( red ){
        r=0.88;
        g=0.35;
        b=0.27;
    };
    double radius;
    cr->save();
    // interior circle
    cr->set_source_rgba(r,g,b,1.0);
    radius = r_point/2.0;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    cr->fill();
    // interior circle
    cr->set_source_rgba(r,g,b,0.8);
    radius = r_point/1.33;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    cr->fill();
    cr->set_source_rgba(r,g,b,0.8);
    radius = r_point;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    //cr->fill();
    cr->stroke();
    cr->set_source_rgba(r,g,b,0.1);
    radius = r_point_shadow;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    cr->stroke();
    cr->restore();

    //LOG_OUT();
};

void Draw::draw_note_off(const Cairo::RefPtr<Cairo::Context>& cr,double x_noteoff, double height){
    /* set the note Off line */
    cr->save();
    cr->set_source_rgba(dash_color[0], dash_color[1], dash_color[2], dash_color[3]);
    cr->set_dash(dash_pattern,dash_offset);
    cr->set_line_width(dash_width+2.0);
    cr->move_to(x_noteoff, 0);
    cr->line_to(x_noteoff, height);
    cr->stroke();
    cr->restore();
};

void Draw::draw_adsr(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height, Glib::ustring type){
    LOG_IN();
    //set origin to bottom left
    cr->translate(0, height);
    cr->scale(1, -1);
    double x_noteoff=(width)*3.0/4.0;
    draw_note_off(cr, x_noteoff, height);

    // Draw ADSR
    // scale factor, to compute x
    height-=2.0*r_point;
    width-=2.0*r_point;
    double x_ratio=( (width ) /400.0);
    double y_ratio=( (height) /99.0);
    // TODO: global output lvl scale the distance
    // put key on key off mark

    /* Draw curve */
    bool r_flag = false;      //draw red point flag for First and last point switch color
    double x=0.0, y=0.0;     // coordonee du point
    x += r_point;
    y += r_point;

    for(uint8_t i=1;i<=4;i++) {
        cr->save();
        cr->set_source_rgba(line_color[0],line_color[1],line_color[2],1.0);
        cr->set_line_width(line_width);
        r_flag = false;
        if (i==1) {
            cr->move_to( x,
                ( y + ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl4_"+type))->get_value()* y_ratio) )
            );
        }else{
            cr->move_to(x, y);    // deplace le curseur
        };
        if(i == 4){
            x = x_noteoff;
            cr->line_to(x, y);      // trace une ligne
            cr->move_to(x, y);
            r_flag = true;
        };
        x += ( ( x_ratio * (
            std::abs( 100.0 - (double)( (get_gwidget<Gtk::SpinButton>("eg_rt"+tostr<uint>(i)+"_"+type))->get_value() + 1.0 ) )
        ) ) ) ;
        y = (r_point) + ( ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<uint>(i)+"_"+type))->get_value() ) * y_ratio ) /*+ line_width*/;
        cr->line_to(x, y);      // trace une ligne
        cr->move_to(x, y);
        cr->stroke();
        cr->restore();
        points[i]={x,y};
        draw_point(cr,x,y, width,r_flag);
    };
    //draw first point at last to covert line
    x = r_point;
    y = r_point + ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl4_"+type))->get_value() * y_ratio );
    points[0]={x,y};
    draw_point(cr, x, y, width, true);
    LOG_OUT();
};

void Draw::on_draw_op_event(const Cairo::RefPtr<Cairo::Context>& cr,int width, int height, Glib::ustring num_op){
    LOG_IN();
    if (cr && (width != 0) && (height != 0) ){
        double wdth=(double)width, hght=(double)height;
        draw_background(cr);
        draw_grid( cr, wdth, hght);
        draw_adsr( cr, wdth, hght, "op"+num_op );
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op"+num_op))->queue_draw();
    };
    LOG_OUT();
};

void Draw::on_eg_rt1_op1_event() {
    LOG_IN();
    eg_rt[0].val=(get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->get_value();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};
void Draw::on_eg_rt2_op1_event() {
    LOG_IN();
    eg_rt[1].val=(get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->get_value();
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};
void Draw::on_eg_rt3_op1_event() {
    LOG_IN();
    eg_rt[2].val=(get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->get_value();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};
void Draw::on_eg_rt4_op1_event() {
    LOG_IN();
    eg_rt[3].val=(get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->get_value();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};
void Draw::on_eg_lvl1_op1_event() {
    LOG_IN();
    eg_lvl[0].val=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->get_value();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};
void Draw::on_eg_lvl2_op1_event() {
    LOG_IN();
    eg_lvl[1].val=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->get_value();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};
void Draw::on_eg_lvl3_op1_event() {
    LOG_IN();
    eg_lvl[2].val=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->get_value();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};
void Draw::on_eg_lvl4_op1_event() {
    LOG_IN();
    eg_lvl[3].val=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->get_value();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};

/* UI SIGNALS CONNECTION */
void Draw::attach_signals(){	 
    LOG_IN();
    controller_mouse_moove->signal_motion().connect(
        [this](double x, double y) {
            auto widget = controller_mouse_button->get_widget();
            if (widget) {
                mouse_moove(x, y, widget->get_buildable_id());
            };
        }
    );
    controller_mouse_button->signal_pressed().connect(
        [this](int n_press, double x, double y) {
            auto widget = controller_mouse_button->get_widget();
            if (widget) {
                mouse_click(n_press, x, y, widget->get_buildable_id());
            };
        }
    );
    controller_mouse_button->signal_released().connect(
        [this](int n_press, double x, double y) {
            auto widget = controller_mouse_button->get_widget();
            if (widget) {
                mouse_release(n_press, x, y, widget->get_buildable_id());
            };
        }
    );

    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->add_controller(controller_mouse_button);
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->add_controller(controller_mouse_moove);
	/* generale  */
    // spinbutton change
    slot_eg_rt1_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Draw::on_eg_rt1_op1_event));
	slot_eg_rt2_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Draw::on_eg_rt2_op1_event));
	slot_eg_rt3_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Draw::on_eg_rt3_op1_event));
	slot_eg_rt4_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Draw::on_eg_rt4_op1_event));
    slot_eg_lvl1_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Draw::on_eg_lvl1_op1_event));
	slot_eg_lvl2_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Draw::on_eg_lvl2_op1_event));
	slot_eg_lvl3_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Draw::on_eg_lvl3_op1_event));
	slot_eg_lvl4_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Draw::on_eg_lvl4_op1_event));
    // drawing area event draw
    /* Drawing area for each operator */
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Draw::on_draw_op_event), Glib::ustring("1") )
    );
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->signal_resize().connect(
        sigc::mem_fun(*this, &Draw::onresize)
    );
};

void Draw::dettach_signals(){	 
    LOG_IN();	
	LOG_OUT(); 
};

void Draw::block_all(){
	/* block ui */
    //slot_eg_rt1_op1.block(true);
};

void Draw::unblock_all(){
	/* unblock ui */
	//slot_eg_rt1_op1.unblock();
};
