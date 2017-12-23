loe::eereplace({(\W)memcmp},{"$1SDL_memcmp"});
loe::replace({SET_INI_TRUE},{1});
loe::replace({SET_INI_FALSE},{0});
loe::replace({SET_INI_ASSERT},{SDL_assert});
loe::replace({SET_INI_BOOLEAN},{Uint8});
loe::replace({SET_INI_INT64},{Sint64});
loe::replace({SET_INI_UINT64},{Uint64});
loe::replace({LOE_STACK_MALLOC},{SDL_malloc});
loe::replace({LOE_STACK_REALLOC},{SDL_realloc});
loe::replace({LOE_STACK_FREE},{SDL_free});
loe::replace({LOE_STACK_LOG_OVERFLOW_ERROR},{clog("Переизбыток данных.")});
loe::replace({LOE_STACK_LOG_CRITICAL_MALLOC_ERROR},{clog("Выделение памяти.")});
loe::replace({LOE_STACK_LOG_CRITICAL_REALLOC_ERROR},{clog("Перевыделение памяти.")});
loe::replace({LOE_STACK_ASSERT},{SDL_assert});
#include "config.h"
#include <SDL.h>
#include <SDL_image.h>
#include "boxy.h"

#define TOSTR2(x) #x
#define TOSTR(x) TOSTR2(x)

#define elog(f,args...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,\
	SDL_FILE"["TOSTR(SDL_LINE)"]:%s->"f,SDL_FUNCTION,##args)
#define clog(f,args...) SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,\
	SDL_FILE"["TOSTR(SDL_LINE)"]:%s->"f,SDL_FUNCTION,##args)
#define wlog(f,args...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,\
	SDL_FILE"["TOSTR(SDL_LINE)"]:%s->"f,SDL_FUNCTION,##args)
#define ilog(f,args...) SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,\
	SDL_FILE"["TOSTR(SDL_LINE)"]:%s->"f,SDL_FUNCTION,##args)

#define esdl elog("SDL_GetError(): '%s'.",SDL_GetError())
#define eimg elog("IMG_GetError(): '%s'.",IMG_GetError())

#define tmax(t) ((t)-1<0?((t)1<<(sizeof(t)*8-2))-1+((t)1<<(sizeof(t)*8-2)):(t)-1)
#define tmin(t) ((t)-1<0?((t)1<<(sizeof(t)*8-1)):0)
#define tmaxof(e) tmax(typeof(e))
#define tminof(e) tmin(typeof(e))

loe::stack(string){
	::index_type{size_t};
	::length{n};
	::queue{q};
	::array{{char}{i}};
	::allstatic;
	size_t n,q;
	char i[];
}

loe::stack(atlas){
	::index_type{Uint64};
	::length{n};
	::queue{q};
	::array{{struct atlas_record}{i}};
	::allstatic;
	Uint64 n,q;
	struct atlas_record{
		Uint32 x,y,w,h;
		Uint8 ok;
		Uint8 omin,omax;
		Uint32 hash;
		struct string*id;
	}i[];
}

loe::stack(stripes){
	::index_type{Uint64};
	::length{n};
	::queue{q};
	::array{{struct stripes_record}{i}};
	::allstatic;
	Uint64 n,q;
	struct stripes_record{
		Uint32 h_over,h_empty_over,v_over;
		Uint8 empty;
	}i[];
}

loe::stack(boxyheader){
	::index_type{Uint64};
	::length{n_frames};
	::allstatic;
	::array{{Uint64}{offsets}};
	::afterstruct{__attribute__((packed))};
	::hidestruct;
	::storestruct{boxyheader};
	char magic[8];
	Uint8 bigendian;
	Uint32 width,height;
	Uint64 n_frames,offsets[];
}

loe::stack(boxyhitboxes){
	::index_type{Uint64};
	::length{n_boxes};
	::allstatic;
	::array{{struct boxyhitboxes_rect}{boxes}};
	::afterstruct{__attribute__((packed))};
	::hidestruct;
	::storestruct{boxyhitboxes};
	Uint64 n_boxes;
	struct boxyhitboxes_rect{
		Uint32 x,y,w,h;
	}__attribute__((packed)) frame,lazy,boxes[];
}

loe::list(arearects){
	::index_type{Uint64};
	::length{n};
	::queue{q};
	::array{{typeof(((struct arearects*)0)->i[0])}{i}};
	::allstatic;
	::first{first};
	::last{last};
	::prev{prev};
	::next{next};
	::item{{struct arearects_rect}{rect}};
	::efirst{efirst};
	::elast{elast};
	::eprev{eprev};
	::enext{enext};
	Uint64 n,q;
	Uint64 first,last,efirst,elast;
	Uint32 x_origin,y_origin;
	struct{
		Uint64 prev,next,eprev,enext;
		struct arearects_rect{
			Uint32 x,h_over,v_over;
		}rect;
	}i[];
}

enum slurpfile_result{
	slurpfile_ok,slurpfile_error,slurpfile_critical_error
};

enum atlas_record_ok{
	atlas_record_ok_none=0,
	atlas_record_ok_x=1,
	atlas_record_ok_y=2,
	atlas_record_ok_w=4,
	atlas_record_ok_h=8,
	atlas_record_ok_combined=15
};

enum printrw_result{
	printrw_ok,
	printrw_output_error,
	printrw_overflow_error,
	printrw_critical_realloc_error,
	printrw_write_error
};

static SET_INI_BOOLEAN
	opt_show_help=SET_INI_FALSE,opt_show_version=SET_INI_FALSE,
	opt_syntax_error=SET_INI_FALSE,opt_no_sparse=SET_INI_FALSE;

static struct string
	*opt_image=NULL,
	*opt_boxy=NULL,
	*opt_enum=NULL,
	*opt_prefix=NULL,
	*opt_header=NULL;

static Uint8
	opt_omin=1,
	opt_omax=255;

static struct string*tmpstring;

static int slurpfile(struct string**s,const char*f){
	SDL_RWops *r=SDL_RWFromFile(f,"rb");
	if(!r){
		esdl;
		return slurpfile_error;
	}
	Sint64 z=SDL_RWsize(r);
	if(z==-1){
		esdl;
		SDL_RWclose(r);
		return slurpfile_error;
	}
	size_t a;
	if(__builtin_add_overflow(z,1,&a)){
		elog("Файл слишком большой ('%s').",f);
		SDL_RWclose(r);
		return slurpfile_error;
	}
	s[0]->n=0;
	switch(string_occupy(s,a)){
		case string_occupy_ok:
			break;
		case string_occupy_overflow_error:{
			clog("Файл слишком большой ('%s').",f);
			SDL_RWclose(r);
			return slurpfile_error;
		}
		case string_occupy_critical_realloc_error:{
			clog("Перевыделение памяти для содержимого файла ('%s').",f);
			SDL_RWclose(r);
			return slurpfile_critical_error;
		}
	}
	if(SDL_RWread(r,s[0]->i,1,z)!=z){
		elog("Чтение файла '%s'; SDL_GetError(): '%s'.",f,SDL_GetError());
		SDL_RWclose(r);
		return slurpfile_error;
	}
	SDL_RWclose(r);
	s[0]->i[s[0]->n-1]='\0';
	return slurpfile_ok;
}

static Uint32 addrol13(const char*s,size_t z){
	Uint32 hash=0;
	for(size_t l=0;l<z;++l)
		hash=((hash<<13)|(hash>>19))+((unsigned char*)s)[l];
	return hash;
}

set::gperfing{
	%compare-lengths
	%struct-type
	%enum
	%readonly-tables
	%define lookup-function-name atlas_record_attribute_in_word_set
	%define hash-function-name atlas_record_hash
	struct atlas_record_attribute{const char*name;size_t offset;enum atlas_record_ok ok;}
	%%
	x,offsetof(struct atlas_record,x),atlas_record_ok_x
	y,offsetof(struct atlas_record,y),atlas_record_ok_y
	w,offsetof(struct atlas_record,w),atlas_record_ok_w
	h,offsetof(struct atlas_record,h),atlas_record_ok_h
	omin,offsetof(struct atlas_record,omin),atlas_record_ok_none
	omax,offsetof(struct atlas_record,omax),atlas_record_ok_none
}

set::ini_works;

static SET_INI_BOOLEAN opt_report_key(const struct SET_INI_GROUP*g,
const char*gn,size_t gz,const char*kn,size_t kz,const char*v,size_t vz,
SET_INI_INT64 i,enum SET_INI_TYPE t,const void*udata){
	if(!g){
		Uint32 hash=addrol13(gn,gz);
		Uint64 l=0;
#define pool (((struct atlas**)udata)[0])
		for(;hash!=pool->i[l].hash || gz!=pool->i[l].id->n-1 ||
			SDL_memcmp(gn,pool->i[l].id->i,gz);++l);
		const struct atlas_record_attribute *a=atlas_record_attribute_in_word_set(kn,kz);
		if(a){
			if(t==SET_INI_TYPE_SINT64){
				switch(a->offset){
					case offsetof(struct atlas_record,x):
					case offsetof(struct atlas_record,y):{
						if(i>=0 && i<=0xffffffff){
							*((Uint32*)(((ptrdiff_t)&(pool->i[l]))+a->offset))=i;
							pool->i[l].ok|=a->ok;
						}else{
							elog("[%.*s] %.*s - вне диапазона[0..4294967295]",(int)gz,gn,(int)kz,kn);
							opt_syntax_error=SET_INI_TRUE;
						}
						break;
					}
					case offsetof(struct atlas_record,w):
					case offsetof(struct atlas_record,h):{
						if(i>=1 && i<=0xffffffff){
							*((Uint32*)(((ptrdiff_t)&(pool->i[l]))+a->offset))=i;
							pool->i[l].ok|=a->ok;
						}else{
							elog("[%.*s] %.*s - вне диапазона[1..4294967295]",(int)gz,gn,(int)kz,kn);
							opt_syntax_error=SET_INI_TRUE;
						}
						break;
					}
					case offsetof(struct atlas_record,omin):
					case offsetof(struct atlas_record,omax):{
						if(i>=0 && i<=255){
							*((Uint8*)(((ptrdiff_t)&(pool->i[l]))+a->offset))=i;
						}else{
							elog("[%.*s] %.*s - вне диапазона[0..255]",(int)gz,gn,(int)kz,kn);
							opt_syntax_error=SET_INI_TRUE;
						}
						break;
					}
				}
			}else{
				elog("[%.*s] %.*s - неверный тип",(int)gz,gn,(int)kz,kn);
				opt_syntax_error=SET_INI_TRUE;
			}
		}else{
			elog("[%.*s] %.*s - неизвестный атрибут",(int)gz,gn,(int)kz,kn);
			opt_syntax_error=SET_INI_TRUE;
		}
#undef pool
	}else{
		elog("%.*s - неизвестная опция",(int)kz,kn);
		opt_syntax_error=SET_INI_TRUE;
	}
	return SET_INI_TRUE;
}

static SET_INI_BOOLEAN opt_report_group(const char*gn,size_t gz,
const void*udata){
	Uint32 hash=addrol13(gn,gz);
#define pool (((struct atlas**)udata)[0])
	for(Uint64 l=0;l<pool->n;++l){
		if(hash==pool->i[l].hash && gz==pool->i[l].id->n-1 &&
			SDL_memcmp(gn,pool->i[l].id->i,gz)==0)
			return SET_INI_TRUE;
	}
#undef pool
	struct string*s=string_new(gz+1);
	if(!s){
		clog("Выделение памяти.");
		return SET_INI_FALSE;
	}
	string_occupy(&s,gz+1);
	SDL_memcpy(s->i,gn,gz);
	s->i[gz]='\0';
	switch(atlas_push((struct atlas**)udata,(struct atlas_record){
		.id=s,.hash=hash,.omin=opt_omin,.omax=opt_omax,.ok=0
	})){
		case atlas_occupy_ok:
			break;
		case atlas_occupy_overflow_error:{
			clog("Переизбыток данных.");
			return SET_INI_FALSE;
		}
		case atlas_occupy_critical_realloc_error:{
			clog("Перевыделение памяти.");
			return SET_INI_FALSE;
		}
	}
	return SET_INI_TRUE;
}

static const struct opt_group*opt_in_word_set(register const char*,register size_t);

set::ini_info(opt){
	empty{
		names ""
		keys{
			bool{
				names "--help" "h" "--version" "v" "--no-sparse" "ns"
				decl "SET_INI_BOOLEAN*pb;"
				atts ".bool={&opt_show_help}" ".bool={&opt_show_help}"
					".bool={&opt_show_version}" ".bool={&opt_show_version}"
					".bool={&opt_no_sparse}" ".bool={&opt_no_sparse}"
				onload{
					if(t==SET_INI_TYPE_BOOLEAN){
						k->bool.pb[0]=SET_INI_TRUE;
					}else{
						elog("%.*s - неверный тип",(int)kz,kn);
						opt_syntax_error=SET_INI_TRUE;
					}
					return SET_INI_TRUE;
				}
			}
			str{
				names "--image" "i" "--boxy" "b" "--enum" "e" "--prefix" "p"
					"--header" "r"
				decl "struct string**ps;"
				atts ".str={&opt_image}" ".str={&opt_image}"
					".str={&opt_boxy}" ".str={&opt_boxy}"
					".str={&opt_enum}" ".str={&opt_enum}"
					".str={&opt_prefix}" ".str={&opt_prefix}"
					".str={&opt_header}" ".str={&opt_header}"
				onload{
					if(t!=SET_INI_TYPE_BOOLEAN){
						if(!k->str.ps[0]){
							k->str.ps[0]=string_new(vz+1);
							string_occupy(k->str.ps,vz+1);
						}else{
							k->str.ps[0]->n=0;
							switch(string_occupy(k->str.ps,vz+1)){
								case string_occupy_ok:
									break;
								case string_occupy_overflow_error:{
									clog("Переизбыток данных.");
									return SET_INI_FALSE;
								}
								case string_occupy_critical_realloc_error:{
									clog("Перевыделение памяти.");
									return SET_INI_FALSE;
								}
							}
						}
						SDL_memcpy(k->str.ps[0]->i,v,vz);
						k->str.ps[0]->i[vz]='\0';
					}else{
						elog("%.*s - неверный тип.",(int)kz,kn);
						opt_syntax_error=SET_INI_TRUE;
					}
					return SET_INI_TRUE;
				}
			}
			conf{
				names "--conf" "c"
				onload{
					if(t!=SET_INI_TYPE_BOOLEAN){
						tmpstring->n=0;
						switch(string_occupy(&tmpstring,vz+1)){
							case string_occupy_ok:{
								SDL_memcpy(tmpstring->i,v,vz);
								tmpstring->i[vz]='\0';
								switch(slurpfile(&tmpstring,tmpstring->i)){
									case slurpfile_ok:{
										switch(set_ini_parse_string((SET_INI_GROUP_IN_WORD_SET)opt_in_word_set,
											tmpstring->i,opt_report_group,opt_report_key,udata)){
											case SET_INI_PARSER_OK:{
												break;
											}
											case SET_INI_PARSER_CANCELLED:{
												clog("При сканировании файла '%.*s'.",(int)vz,v);
												return SET_INI_FALSE;
											}
											case SET_INI_PARSER_UTF8_ERROR:{
												elog("Ошибка UTF-8 кодировки '%.*s'.",(int)vz,v);
												opt_syntax_error=SET_INI_TRUE;
												break;
											}
										}
										break;
									}
									case slurpfile_error:{
										elog("slurpfile здесь.");
										break;
									}
									case slurpfile_critical_error:{
										clog("slurpfile здесь.");
										return SET_INI_FALSE;
									}
								}
								break;
							}
							case string_occupy_overflow_error:{
								clog("Переизбыток данных.");
								return SET_INI_FALSE;
							}
							case string_occupy_critical_realloc_error:{
								clog("Перевыделение памяти.");
								return SET_INI_FALSE;
							}
						}
					}else{
						elog("%.*s - неверный тип.",(int)kz,kn);
						opt_syntax_error=SET_INI_TRUE;
					}
					return SET_INI_TRUE;
				}
			}
			opacity{
				names "--opacity-max" "omax" "--opacity-min" "omin"
				decl "Uint8*po;"
				atts ".opacity={&opt_omax}" ".opacity={&opt_omax}"
					".opacity={&opt_omin}" ".opacity={&opt_omin}"
				onload{
					if(t==SET_INI_TYPE_SINT64){
						if(i>=0 && i<=255){
							k->opacity.po[0]=i;
						}else{
							elog("%.*s - вне диапазона.",(int)kz,kn);
							opt_syntax_error=SET_INI_TRUE;
						}
					}else{
						elog("%.*s - неверный тип.",(int)kz,kn);
						opt_syntax_error=SET_INI_TRUE;
					}
					return SET_INI_TRUE;
				}
			}
		}
	}
}

static int checkatlas(struct atlas*a){
	Uint64 l=0;
	do{
		if(a->i[l].ok!=atlas_record_ok_combined){
			if(!(a->i[l].ok&atlas_record_ok_x))
				elog("[%s] x - отсутствует.",a->i[l].id->i);
			if(!(a->i[l].ok&atlas_record_ok_y))
				elog("[%s] y - отсутствует.",a->i[l].id->i);
			if(!(a->i[l].ok&atlas_record_ok_w))
				elog("[%s] w - отсутствует.",a->i[l].id->i);
			if(!(a->i[l].ok&atlas_record_ok_h))
				elog("[%s] h - отсутствует.",a->i[l].id->i);
			return 0;
		}
	}while(++l<a->n);
	return 1;
}

static enum printrw_result printrw(SDL_RWops *w,const char*fmt,...){
	va_list l;
	va_start(l,fmt);
	int z=SDL_vsnprintf(NULL,0,fmt,l);
	if(z==-1){
		va_end(l);
		elog("Ошибка форматирования данных с форматирующей строкой '%s'.",fmt);
		return printrw_output_error;
	}
	va_end(l);
	tmpstring->n=0;
	switch(string_occupy(&tmpstring,z+1)){
		case string_occupy_ok:
			break;
		case string_occupy_overflow_error:{
			clog("Переизбыток данных.");
			return printrw_overflow_error;
		}
		case string_occupy_critical_realloc_error:{
			clog("Перевыделение памяти.");
			return printrw_critical_realloc_error;
		}
	}
	va_start(l,fmt);
	SDL_vsnprintf(tmpstring->i,tmpstring->n,fmt,l);
	va_end(l);
	if(SDL_RWwrite(w,tmpstring->i,z,1)!=1){
		elog("Ошибка вывода данных с форматирующей строкой '%s'.",fmt);
		return printrw_write_error;
	}
	return printrw_ok;
}

static Uint8 geta(SDL_Surface*sur,Uint32 x,Uint32 y){
	//~ ilog("%"PRIu32",%"PRIu32",%i,%i",x,y,sur->w,sur->h);
	SDL_assert(x>=0 && x<sur->w && y>=0 && y<sur->h);
	int sz=sur->pitch/sur->w;
	Uint32 pixel=((Uint8*)(sur->pixels))[sz*(x+y*sur->w)];
	for(int z=1;z<sz;++z)
		pixel|=(Uint32)(((Uint8*)(sur->pixels))[sz*(x+y*sur->w)+z])<<(z*8);
	Uint8 r,g,b,a;
	SDL_GetRGBA(pixel,sur->format,&r,&g,&b,&a);
	return a;
}

static void stripes_h_over_fill(struct stripes*s,Uint32 w,Uint32 y,Uint32 start,Uint32 over){
	do{
		s->i[start+y*w].h_over=over;
	}while(++start<over);
}

static void stripes_v_over_fill(struct stripes*s,Uint32 w,Uint32 x,Uint32 start,Uint32 over){
	do{
		s->i[x+start*w].v_over=over;
	}while(++start<over);
}

static int stripes_scan(struct stripes**s,SDL_Surface*sur,const struct atlas_record r){
	//~ s[0]->n=0;
	//~ ilog("%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32,r.x,r.y,r.w,r.h);
	switch(stripes_occupy(s,r.w*r.h)){
		case stripes_occupy_ok:
			break;
		case stripes_occupy_overflow_error:{
			clog("Переизбыток данных.");
			return 0;
		}
		case stripes_occupy_critical_realloc_error:{
			clog("Перевыделение памяти.");
			return 0;
		}
	}
	Uint32 y=0,x,start;
	do{
		x=0;
		Uint8 a=geta(sur,x+r.x,y+r.y);
		if(a>=r.omin && a<=r.omax){
l_con1:		start=x;
			do{
				s[0]->i[x+y*r.w].empty=0;
				if(++x==r.w){
					stripes_h_over_fill(s[0],r.w,y,start,x);
					goto l_con0;
				}
				//~ if(x+1==r.w){
					//~ s[0]->i[x+y*r.w].h_over=r.w;
					//~ ++x;
					//~ goto l_con0;
				//~ }
				a=geta(sur,x+r.x,y+r.y);
			}while(a>=r.omin && a<=r.omax);
			stripes_h_over_fill(s[0],r.w,y,start,x);
			//~ Uint32 tx=x-1;
			//~ while(1){
				//~ s[0]->i[tx+y*r.w].h_over=x;
				//~ if(tx==start)
					//~ break;
				//~ tx--;
			//~ }
			//~ for(Uint32 tx=x-1;tx>=start;--tx)
				//~ s[0]->i[tx+y*r.w].h_over=x;
		}
		start=x;
		do{
			s[0]->i[x+y*r.w].empty=1;
			if(++x==r.w){
				s[0]->i[start+y*r.w].h_empty_over=x;
				goto l_con0;
			}
			a=geta(sur,x+r.x,y+r.y);
		}while(a<r.omin || a>r.omax);
		s[0]->i[start+y*r.w].h_empty_over=x;
		goto l_con1;
l_con0:;
	}while(++y<r.h);
	x=0;
	do{
		y=0;
		do{
			if(!s[0]->i[x+y*r.w].empty){
				start=y;
				do{
					if(++y==r.h){
						stripes_v_over_fill(s[0],r.w,x,start,y);
						goto l_con2;
					}
				}while(!s[0]->i[x+y*r.w].empty);
				stripes_v_over_fill(s[0],r.w,x,start,y);
				//~ Uint32 ty=y-1;
				//~ while(1){
					//~ s[0]->i[x+ty*r.w].v_over=y;
					//~ if(ty==start)
						//~ break;
					//~ ty--;
				//~ }
				//~ for(Uint32 ty=y-1;ty>=start;--ty)
					//~ s[0]->i[x+ty*r.w].v_over=y;
			}
		}while(++y!=r.h);
l_con2:;
	}while(++x!=r.w);
	return 1;
}

static void arearects_cut(struct arearects*a,Uint64 i){
	Uint64 l=a->last;
	while(1){
		arearects_remove(a,l);
		if(l==i)
			break;
		l=a->last;
	}
}

static enum arearects_occupy_result areascan(struct arearects**a,
struct stripes*s,Uint32 sw,Uint32 sx,Uint32 h_over,Uint32 v_over){
	for(Uint32 x=sx+1;x<h_over;++x){
		if(s->i[x+sw*a[0]->y_origin].v_over<v_over){
			//~ if(a[0]->y_origin==91 && a[0]->x_origin==101){
				//~ ilog("inside: %"PRIu32",%"PRIu32",%"PRIu32,sx,x,v_over);
			//~ }
			enum arearects_occupy_result e=arearects_append(a,(struct arearects_rect){
				.x=sx,.h_over=x,.v_over=v_over
			});
			if(e!=arearects_occupy_ok)
				return e;
			return areascan(a,s,sw,x,h_over,s->i[x+sw*a[0]->y_origin].v_over);
		}
	}
	//~ if(a[0]->y_origin==91 && a[0]->x_origin==101){
		//~ ilog("outside: %"PRIu32",%"PRIu32",%"PRIu32,sx,h_over,v_over);
	//~ }
	return arearects_append(a,(struct arearects_rect){
		.x=sx,.h_over=h_over,.v_over=v_over
	});
}

static void arearects_v_over_lowered(struct arearects*a,Uint64 i,Uint32 v_over){
	while(1){
		if(a->i[i].rect.v_over>v_over)
			a->i[i].rect.v_over=v_over;
		if(i==a->last)
			break;
		i=a->i[i].next;
	}
}

static enum arearects_occupy_result arearects_insert(struct arearects**a,
Uint64 after,struct arearects_rect r){
	enum arearects_occupy_result e=arearects_append(a,r);
	if(e==arearects_occupy_ok){
		if(a[0]->i[after].next!=a[0]->last)
			arearects_swap(a[0],a[0]->last,a[0]->i[after].next);
	}
	return e;
}

static enum arearects_occupy_result getmaxrect(Uint32 *nw,Uint32 *nh,
struct arearects**a,struct boxyhitboxes*b){
	Uint64 l=0;
	for(;l<b->n_boxes;++l){
		Uint64 m=a[0]->first;
		while(1){
			if(
				a[0]->y_origin<b->boxes[l].y+b->boxes[l].h &&
				a[0]->i[m].rect.v_over>b->boxes[l].y &&
				a[0]->i[m].rect.x<b->boxes[l].x+b->boxes[l].w &&
				a[0]->i[m].rect.h_over>b->boxes[l].x
			){
				if(a[0]->i[m].rect.x>=b->boxes[l].x){
					if(a[0]->y_origin>=b->boxes[l].y){
						arearects_cut(a[0],m);
						break;
					}
					arearects_v_over_lowered(a[0],m,b->boxes[l].y);
				}else{
					if(a[0]->y_origin>=b->boxes[l].y){
						if(m!=a[0]->last)
							arearects_cut(a[0],a[0]->i[m].next);
						a[0]->i[m].rect.h_over=b->boxes[l].x;
						break;
					}
					enum arearects_occupy_result e=arearects_insert(a,m,(struct arearects_rect){
						.x=b->boxes[l].x,.h_over=a[0]->i[m].rect.h_over,.v_over=b->boxes[l].y
					});
					if(e!=arearects_occupy_ok)
						return e;
					a[0]->i[m].rect.h_over=b->boxes[l].x;
					arearects_v_over_lowered(a[0],a[0]->i[m].next,b->boxes[l].y);
				}
			}
			if(m==a[0]->last)
				break;
			m=a[0]->i[m].next;
		}
		if(!a[0]->n)
			break;
	}
	Uint32 mw=0,mh=0;
	if(a[0]->n>0){
		l=a[0]->first;
		while(1){
			if((a[0]->i[l].rect.h_over-a[0]->x_origin)*(a[0]->i[l].rect.v_over-a[0]->y_origin)>mh*mw){
				mw=a[0]->i[l].rect.h_over-a[0]->x_origin;
				mh=a[0]->i[l].rect.v_over-a[0]->y_origin;
			}
			if(l==a[0]->last)
				break;
			l=a[0]->i[l].next;
		}
	}
	*nw=mw;
	*nh=mh;
	return arearects_occupy_ok;
}

static int boxyhitboxes_scan(struct boxyhitboxes**ph,Uint64 *pq,struct stripes*s,struct arearects**a,
Uint32 w,Uint32 h){
	Sint64 lzx=w,lzy=h,lzox= -1,lzoy= -1;
	do{
		struct boxyhitboxes_rect max;
		max.w=max.h=0;
		Uint32 x,y=0;
		do{
			x=0;
			do{
				if(s->i[x+y*w].empty)
					x=s->i[x+y*w].h_empty_over;
				if(x==w)
					break;
				a[0]->x_origin=x;
				a[0]->y_origin=y;
				//~ if(x==101 && y==91){
					//~ ilog("n=%"PRIu64,a[0]->n);
				//~ }
				if(areascan(a,s,w,x,s->i[x+y*w].h_over,s->i[x+y*w].v_over)!=arearects_occupy_ok)
					return 0;
				//~ if(x==101 && y==91){
					//~ ilog("areascan_start:");
					//~ Uint64 k=a[0]->first;
					//~ while(1){
						//~ ilog("%"PRIu32",%"PRIu32",%"PRIu32,a[0]->i[k].rect.x,a[0]->i[k].rect.h_over,a[0]->i[k].rect.v_over);
						//~ if(k==a[0]->last)
							//~ break;
						//~ k=a[0]->i[k].next;
					//~ }
					//~ ilog("areascan_end");
				//~ }
				Uint32 nw,nh;
				if(getmaxrect(&nw,&nh,a,ph[0])!=arearects_occupy_ok)
					return 0;
				SDL_assert(nw<=512 && nh<=512);
				if(max.w*max.h<nw*nh){
					max.x=x;
					max.y=y;
					max.w=nw;
					max.h=nh;
				}
				if(a[0]->n>0)
					arearects_cut(a[0],a[0]->first);
				//~ if(++x==w)
					//~ break;
			}while(++x!=w);
		}while(++y<h);
		if(!max.w){
			ph[0]->lazy=(struct boxyhitboxes_rect){
				lzx,lzy,lzox-lzx,lzoy-lzy
			};
			return 1;
		}
		switch(boxyhitboxes_push(ph,pq,max)){
			case boxyhitboxes_occupy_ok:{
				lzx=max.x<lzx?max.x:lzx;
				lzy=max.y<lzy?max.y:lzy;
				lzox=max.x+max.w>lzox?max.x+max.w:lzox;
				lzoy=max.y+max.h>lzoy?max.y+max.h:lzoy;
				//~ ilog("%"PRIu32",%"PRIu32",%"PRIu32",%"PRIu32,max.x,max.y,max.w,max.h);
				//~ SDL_assert(0);
				break;
			}
			case boxyhitboxes_occupy_overflow_error:{
				clog("Переизбыток данных.");
				return 0;
			}
			case boxyhitboxes_occupy_critical_realloc_error:{
				clog("Перевыделение памяти.");
				return 0;
			}
		}
	}while(1);
}

#ifdef _DEBUGIMAGE
static Uint32 rand_seed;
static Uint32 getrand(){
	return rand_seed=(1103515245*rand_seed+12345)%0x80000000;
}
#endif //_DEBUGIMAGE

static int boxygensurw(SDL_Surface*sur,SDL_RWops*boxy,
SDL_RWops*enumfile,const char*prefix,struct atlas*pool){
	struct boxyheader*bh=boxyheader_new(pool->n);
	if(!bh)
		return 0;
	struct boxyhitboxes*hb=boxyhitboxes_new(127);
	if(!hb){
		boxyheader_free(bh);
		return 0;
	}
	struct stripes*s=stripes_new(512);
	if(!s){
		boxyhitboxes_free(hb);
		boxyheader_free(bh);
		return 0;
	}
	struct arearects*a=arearects_list_new(127);
	if(!a){
		stripes_free(s);
		boxyhitboxes_free(hb);
		boxyheader_free(bh);
		return 0;
	}
	if(opt_no_sparse==SET_INI_TRUE){
		if(SDL_RWwrite(boxy,SDL_memset(bh,0,sizeof(*bh)+8*pool->n),
				sizeof(*bh)+8*pool->n,1)!=1){
			esdl;
			arearects_free(a);
			stripes_free(s);
			boxyhitboxes_free(hb);
			boxyheader_free(bh);
			return 0;
		}
	}else if(SDL_RWseek(boxy,sizeof(*bh)+8*pool->n,RW_SEEK_SET)==-1){
		esdl;
		arearects_free(a);
		stripes_free(s);
		boxyhitboxes_free(hb);
		boxyheader_free(bh);
		return 0;
	}
	if(printrw(enumfile,"#ifndef __%s_INCLUDED\n#define __%s_INCLUDED\n\nenum %s{",
		prefix,prefix,prefix)!=printrw_ok){
		arearects_free(a);
		stripes_free(s);
		boxyhitboxes_free(hb);
		boxyheader_free(bh);
		return 0;
	}
#ifdef _DEBUGIMAGE
	SDL_Surface *osur=SDL_CreateRGBSurface(0,sur->w,sur->h,32,0xff,0xff00,0xff0000,0xff000000);
	if(!osur){
		esdl;
		arearects_free(a);
		stripes_free(s);
		boxyhitboxes_free(hb);
		boxyheader_free(bh);
		return 0;
	}
#endif //_DEBUGIMAGE
	Uint64 l=0,hbque=127,bhque=pool->n,offset=0;
	while(1){
		if(!stripes_scan(&s,sur,pool->i[l])){
			arearects_free(a);
			stripes_free(s);
			boxyhitboxes_free(hb);
			boxyheader_free(bh);
			return 0;
		}
		if(!boxyhitboxes_scan(&hb,&hbque,s,&a,pool->i[l].w,pool->i[l].h)){
			arearects_free(a);
			stripes_free(s);
			boxyhitboxes_free(hb);
			boxyheader_free(bh);
			return 0;
		}
		boxyheader_push(&bh,&bhque,offset);
		//~ switch(boxyheader_push(&bh,&bhque,offset)){
			//~ case boxyheader_occupy_ok:{
				//~ break;
			//~ }
			//~ case boxyheader_occupy_overflow_error:{
				//~ stripes_free(s);
				//~ boxyhitboxes_free(hb);
				//~ boxyheader_free(bh);
				//~ return 0;
			//~ }
			//~ case boxyheader_occupy_critical_realloc_error:{
				//~ stripes_free(s);
				//~ boxyhitboxes_free(hb);
				//~ boxyheader_free(bh);
				//~ return 0;
			//~ }
		//~ }
		hb->frame=(struct boxyhitboxes_rect){
			pool->i[l].x,
			pool->i[l].y,
			pool->i[l].w,
			pool->i[l].h
		};
		if(SDL_RWwrite(boxy,hb,sizeof(*hb)+sizeof(hb->boxes[0])*hb->n_boxes,1)!=1){
			esdl;
			arearects_free(a);
			stripes_free(s);
			boxyhitboxes_free(hb);
			boxyheader_free(bh);
			return 0;
		}
		if(printrw(enumfile,"\n\t%s_%s,",prefix,pool->i[l].id->i)!=printrw_ok){
			arearects_free(a);
			stripes_free(s);
			boxyhitboxes_free(hb);
			boxyheader_free(bh);
			return 0;
		}
#ifdef _DEBUGIMAGE
		for(Uint64 m=0;m<hb->n_boxes;++m){
			SDL_Rect rc={
				pool->i[l].x+hb->boxes[m].x,
				pool->i[l].y+hb->boxes[m].y,
				hb->boxes[m].w,hb->boxes[m].h
			};
			if(SDL_FillRect(osur,&rc,0xff000000+(getrand()&0xffffff))<0){
				esdl;
				arearects_free(a);
				stripes_free(s);
				boxyhitboxes_free(hb);
				boxyheader_free(bh);
				return 0;
			}
		}
#endif //_DEBUGIMAGE
		if(++l==pool->n)
			break;
		if(__builtin_add_overflow(offset,sizeof(*hb)+sizeof(hb->boxes[0])*hb->n_boxes,&offset)){
			clog("Переизбыток данных.");
			arearects_free(a);
			stripes_free(s);
			boxyhitboxes_free(hb);
			boxyheader_free(bh);
			return 0;
		}
		hb->n_boxes=0;
		s->n=0;
	}
	arearects_free(a);
	stripes_free(s);
	boxyhitboxes_free(hb);
#ifdef _DEBUGIMAGE
	if(IMG_SavePNG(osur,"_debugimage.png")<0){
		eimg;
		boxyheader_free(bh);
		return 0;
	}
	SDL_FreeSurface(osur);
#endif //_DEBUGIMAGE
	if(SDL_RWseek(boxy,0,RW_SEEK_SET)==-1){
		esdl;
		boxyheader_free(bh);
		return 0;
	}
	bh->magic[0]='B';
	bh->magic[1]='O';
	bh->magic[2]='X';
	bh->magic[3]='Y';
	bh->magic[4]='L';
	bh->magic[5]='O';
	bh->magic[6]='E';
	bh->magic[7]='5';
	bh->bigendian=SDL_BYTEORDER==SDL_BIG_ENDIAN;
	bh->width=sur->w;
	bh->height=sur->h;
	if(SDL_RWwrite(boxy,bh,sizeof(*bh)+sizeof(bh->offsets[0])*bh->n_frames,1)!=1){
		esdl;
		boxyheader_free(bh);
		return 0;
	}
	boxyheader_free(bh);
	return printrw(enumfile,"\n\t__%s_length\n};\n\n#endif //__%s_INCLUDED",prefix,prefix)==printrw_ok;
}

static int boxygen(const char*image,const char*boxy,
const char*enumfile,const char*prefix,struct atlas*pool){
	int e=0;
	SDL_Surface*sur=IMG_Load(image);
	if(!sur){
		elog("params(%s,%s,%s,%s),IMG_GetError(): '%s'.",image,boxy,enumfile,prefix,IMG_GetError());
	}else{
		SDL_RWops *b=SDL_RWFromFile(boxy,"wb");
		if(!b){
			elog("params(%s,%s,%s,%s),SDL_GetError(): '%s'.",image,boxy,enumfile,prefix,SDL_GetError());
		}else{
			SDL_RWops *n=SDL_RWFromFile(enumfile,"wb");
			if(!n){
				elog("params(%s,%s,%s,%s),SDL_GetError(): '%s'.",image,boxy,enumfile,prefix,SDL_GetError());
			}else{
				e=boxygensurw(sur,b,n,prefix,pool);
				if(SDL_RWclose(n)<0){
					elog("params(%s,%s,%s,%s),SDL_GetError(): '%s'.",image,boxy,enumfile,prefix,SDL_GetError());
					e=0;
				}
			}
			if(SDL_RWclose(b)<0){
				elog("params(%s,%s,%s,%s),SDL_GetError(): '%s'.",image,boxy,enumfile,prefix,SDL_GetError());
				e=0;
			}
		}
		SDL_FreeSurface(sur);
	}
	return e;
}

int main(int i,char**v){
	int exit_code=1;
	if(SDL_Init(0)<0){
		esdl;
	}else{
		if(i>1){
			struct atlas*pool=atlas_new(127);
			if(!pool){
				clog("Выделение памяти.");
			}else{
				if(!string_newout(&tmpstring,127)){
					clog("Выделение памяти.");
				}else{
					switch(set_ini_parse_cmd((SET_INI_GROUP_IN_WORD_SET)opt_in_word_set,
					i-1,(const char*const*)v+1,opt_report_group,opt_report_key,&pool)){
						case SET_INI_PARSER_OK:{
							if(opt_syntax_error==SET_INI_FALSE){
								if(opt_show_help==SET_INI_TRUE || opt_show_version==SET_INI_TRUE){
									if(opt_show_help==SET_INI_TRUE){
										ilog("\nИспользование$ %s опции \\[sprite1\\] x=0 y=0 w=100 h=100 [omin=1] [omax=255] \\[spriteN\\] ... \\[\\] опции\n+-------------+------------+--------------+--------------+\n|  опция №1   |  опция №2  | тип/диапазон |   описание   |\n+-------------+------------+--------------+--------------+\n|   --help    |     h      |   boolean    |   показать   |\n|             |            |              |   справку    |\n+-------------+------------+--------------+--------------+\n|  --version  |     v      |   boolean    |   показать   |\n|             |            |              |    версию    |\n+-------------+------------+--------------+--------------+\n|             |            |              |  заполнять   |\n| --no-sparse |     ns     |   boolean    |    нулями    |\n|             |            |              | пропущенные  |\n|             |            |              |    секции    |\n+-------------+------------+--------------+--------------+\n|   --image   |     i      |    string    |   вводное    |\n|             |            |              | изображение  |\n+-------------+------------+--------------+--------------+\n|             |            |              |   файл для   |\n|   --boxy    |     b      |    string    |  сохранения  |\n|             |            |              |  хитбоксов   |\n+-------------+------------+--------------+--------------+\n|             |            |              | Си-файл для  |\n|   --enum    |     e      |    string    |  сохранения  |\n|             |            |              |  энумераций  |\n+-------------+------------+--------------+--------------+\n|  --prefix   |     p      |    string    |  Си-префикс  |\n+-------------+------------+--------------+--------------+\n|             |            |              |  прочитать   |\n|   --conf    |     c      |    string    | настройки из |\n|             |            |              |  INI-файла   |\n+-------------+------------+--------------+--------------+\n|             |            |              |  Си-файл со  |\n|  --header   |     r      |    string    | структурами  |\n|             |            |              |              |\n+-------------+------------+--------------+--------------+\n|             |            |              |верхний предел|\n|--opacity-max|    omax    |int64/[0..255]|непрозрачности|\n|             |            |              |              |\n+-------------+------------+--------------+--------------+\n|--opacity-min|    omin    |int64/[0..255]|нижний предел |\n|             |            |              |непрозрачности|\n+-------------+------------+--------------+--------------+\n",v[0]);
										ilog("\n+------------+---------------------+---------------------+\n|  Атрибуты  |    тип/диапазон     |      описание       |\n|  спрайта   |                     |                     |\n+------------+---------------------+---------------------+\n|     x      |int64/[0..4294967295]|     X-смещение      |\n+------------+---------------------+---------------------+\n|     y      |int64/[0..4294967295]|     Y-смещение      |\n+------------+---------------------+---------------------+\n|     w      |int64/[0..4294967295]|    ширина фрейма    |\n|            |                     |                     |\n+------------+---------------------+---------------------+\n|     h      |int64/[0..4294967295]|    высота фрейма    |\n|            |                     |                     |\n+------------+---------------------+---------------------+\n|            |                     |   верхний предел    |\n|    omin    |   int64/[0..255]    |   непрозрачности    |\n|            |                     |      фрейма(не      |\n|            |                     |    обязательно)     |\n+------------+---------------------+---------------------+\n|            |                     |    нижний предел    |\n|    omax    |   int64/[0..255]    |   непрозрачности    |\n|            |                     |      фрейма(не      |\n|            |                     |    обязательно)     |\n+------------+---------------------+---------------------+\n");
									}
									if(opt_show_version==SET_INI_TRUE)
										ilog(PACKAGE_VERSION);
								}else{
									if(pool->n>0){
										if(checkatlas(pool)){
											if(opt_image && opt_boxy && opt_enum && opt_prefix){
												if(IMG_Init(-1)==0){
													eimg;
												}else{
#ifdef _DEBUGIMAGE
													rand_seed=SDL_GetTicks();
#endif //_DEBUGIMAGE
													exit_code=!boxygen(opt_image->i,opt_boxy->i,opt_enum->i,opt_prefix->i,pool);
													IMG_Quit();
												}
											}else{
												if(!opt_image)
													elog("Вводное изображение не указано.");
												if(!opt_boxy)
													elog("Файл для сохранения хитбоксов не указан.");
												if(!opt_enum)
													elog("Файл для сохранения Си-перечисления не указан.");
												if(!opt_prefix)
													elog("Префикс для Си-перечисления не указан.");
											}
										}
									}else if(!opt_header){
										elog("Не указано ни одного фрейма.");
									}
									if(opt_header){
										SDL_RWops *hdr=SDL_RWFromFile(opt_header->i,"wb");
										if(!hdr){
											esdl;
										}else{
											static const char data[]="#ifndef BOXY_HEADER_INCLUDED\n#define BOXY_HEADER_INCLUDED\n#include <SDL.h>\n\nstruct boxyheader\n{\n\n  char magic[8];\n  Uint8 bigendian;\n  Uint32 width, height;\n  Uint64 n_frames, offsets[];\n} __attribute__ ((packed));\n\nstruct boxyhitboxes\n{\n\n  Uint64 n_boxes;\n  struct boxyhitboxes_rect\n  {\n    Uint32 x, y, w, h;\n  } __attribute__ ((packed)) frame, lazy, boxes[];\n} __attribute__ ((packed));\n\n#endif\n";
											if(SDL_RWwrite(hdr,data,sizeof(data)-1,1)!=1){
												elog("Ввод данных в файл '%s'. SDL_GetError(): '%s'.",opt_header->i,SDL_GetError());
											}else{
												exit_code=0;
											}
											if(SDL_RWclose(hdr)<0){
												elog("При закрытии файла '%s'. SDL_GetError(): '%s'.",opt_header->i,SDL_GetError());
												exit_code=1;
											}
										}
									}
								}
							}
							break;
						}
						case SET_INI_PARSER_UTF8_ERROR:{
							elog("Ошибка UTF-8 кодировки.");
							break;
						}
						case SET_INI_PARSER_CANCELLED:{
							break;
						}
					}
					if(opt_header)
						string_free(opt_header);
					if(opt_image)
						string_free(opt_image);
					if(opt_boxy)
						string_free(opt_boxy);
					if(opt_enum)
						string_free(opt_enum);
					if(opt_prefix)
						string_free(opt_prefix);
					string_free(tmpstring);
				}
				for(Uint64 l=0;l<pool->n;++l)
					string_free(pool->i[l].id);
				atlas_free(pool);
			}
		}else{
			elog("Параметры не указаны, используйте '--help' для получения справки.");
		}
		SDL_Quit();
	}
	return exit_code;
}
