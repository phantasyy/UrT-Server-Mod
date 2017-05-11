/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

qboolean	scr_initialized;		// ready to draw

cvar_t		*cl_timegraph;
cvar_t		*cl_debuggraph;
cvar_t		*cl_graphheight;
cvar_t		*cl_graphscale;
cvar_t		*cl_graphshift;

cvar_t		*cl_drawclock;
cvar_t		*cl_drawclockcolor;
cvar_t		*cl_drawclockfontsize;
cvar_t		*cl_drawclockposx;
cvar_t		*cl_drawclockposy;
cvar_t		*cl_drawSpree;
/*
================
SCR_DrawNamedPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	assert( width != 0 );

	hShader = re.RegisterShader( picname );
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}


/*
================
SCR_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void SCR_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	float	xscale;
	float	yscale;

#if 0
		// adjust for wide screens
		if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 ) {
			*x += 0.5 * ( cls.glconfig.vidWidth - ( cls.glconfig.vidHeight * 640 / 480 ) );
		}
#endif

	// scale for screen sizes
	xscale = cls.glconfig.vidWidth / 640.0;
	yscale = cls.glconfig.vidHeight / 480.0;
	if ( x ) {
		*x *= xscale;
	}
	if ( y ) {
		*y *= yscale;
	}
	if ( w ) {
		*w *= xscale;
	}
	if ( h ) {
		*h *= yscale;
	}
}

/*
================
SCR_FillRect

Coordinates are 640*480 virtual values
=================
*/
void SCR_FillRect( float x, float y, float width, float height, const float *color ) {
	re.SetColor( color );

	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cls.whiteShader );

	re.SetColor( NULL );
}


/*
================
SCR_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	SCR_AdjustFrom640( &x, &y, &width, &height );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}



/*
** SCR_DrawChar
** chars are drawn at 640*480 virtual screen size
*/
static void SCR_DrawChar( int x, int y, float size, int ch ) {
	int row, col;
	float frow, fcol;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -size ) {
		return;
	}

	ax = x;
	ay = y;
	aw = size;
	ah = size;
	SCR_AdjustFrom640( &ax, &ay, &aw, &ah );

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	re.DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cls.charSetShader );
}

/*
** SCR_DrawSmallChar
** small chars are drawn at native screen resolution
*/
void SCR_DrawSmallChar( int x, int y, int ch ) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -SMALLCHAR_HEIGHT ) {
		return;
	}

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	re.DrawStretchPic( x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cls.charSetShader );
}


/*
==================
SCR_DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawStringExt( int x, int y, float size, const char *string, float *setColor, qboolean forceColor ) {
	vec4_t		color;
	const char	*s;
	int			xx;

	// draw the drop shadow
	color[0] = color[1] = color[2] = 0;
	color[3] = setColor[3];
	re.SetColor( color );
	s = string;
	xx = x;
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
			continue;
		}
		SCR_DrawChar( xx+1, y+1, size, *s );
		xx += size;
		s++;
	}


	// draw the colored text
	s = string;
	xx = x;
	re.SetColor( setColor );
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				Com_Memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				re.SetColor( color );
			}
			s += 2;
			continue;
		}
		SCR_DrawChar( xx, y, size, *s );
		xx += size;
		s++;
	}
	re.SetColor( NULL );
}

void SCR_DrawCondensedString( int x, int y, float size, const char *string, float *setColor, qboolean forceColor ) {
	vec4_t		color;
	const char	*s;
	int			xx;

	// draw the drop shadow
	color[0] = color[1] = color[2] = 0;
	color[3] = setColor[3];
	re.SetColor( color );
	s = string;
	xx = x;
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
			continue;
		}
		SCR_DrawChar( xx+1, y+1, size, *s );
		xx += size - (size / 8);
		s++;
	}


	// draw the colored text
	s = string;
	xx = x;
	re.SetColor( setColor );
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				Com_Memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				re.SetColor( color );
			}
			s += 2;
			continue;
		}
		SCR_DrawChar( xx, y, size, *s );
		xx += size - (size / 8);
		s++;
	}
	re.SetColor( NULL );
}

int SCR_CondensedStringWidth(float size, char *s) {
	int len = strlen(s);

	return (int)(size * len - size / 8 * len);
}


void SCR_DrawBigString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qfalse );
}

void SCR_DrawBigStringColor( int x, int y, const char *s, vec4_t color ) {
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qtrue );
}

/*
==================
SCR_DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawSmallStringExt( int x, int y, const char *string, float *setColor, qboolean forceColor ) {
	vec4_t		color;
	const char	*s;
	int			xx;

	// draw the colored text
	s = string;
	xx = x;
	re.SetColor( setColor );
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				Com_Memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				re.SetColor( color );
			}
			s += 2;
			continue;
		}
		SCR_DrawSmallChar( xx, y, *s );
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	re.SetColor( NULL );
}



/*
** SCR_Strlen -- skips color escape codes
*/
static int SCR_Strlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

/*
** SCR_GetBigStringWidth
*/
int SCR_GetSmallStringWidth(const char *str) {
    return SCR_Strlen(str) * SMALLCHAR_WIDTH;
}

/*
** SCR_GetBigStringWidth
*/ 
int	SCR_GetBigStringWidth( const char *str ) {
	return SCR_Strlen( str ) * 16;
}

int SCR_FontWidth(const char *text, float scale) {
	if (!cls.fontFont)
		return;

	int 		 count, len;
	float		 out;
	glyphInfo_t  *glyph;
	float		 useScale;
	const char	 *s    = text;
	fontInfo_t	 *font = &cls.font;

	useScale = scale * font->glyphScale;
	out  = 0;

	if (text) {
		len = strlen(text);
		count = 0;

		while (s && *s && count < len) {
			if (Q_IsColorString(s)) {
				s += 2;
				continue;
			}

			glyph = &font->glyphs[(int)*s];
			out  += glyph->xSkip;
			s++;
			count++;
		}
	}
	return out * useScale;
}


void SCR_DrawFontChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
	if (!cls.fontFont)
		return;

	float  w, h;

	w = width * scale;
	h = height * scale;
	SCR_AdjustFrom640(&x, &y, &w, &h);
	re.DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

/**
 * $(function)
 */
void SCR_DrawFontText(float x, float y, float scale, vec4_t color, const char *text, int style) {
	if (!cls.fontFont)
		return;
	
	int 	 len, count;
	vec4_t		 newColor;
	vec4_t		 black = {0.0f, 0.0f, 0.0f, 1.0f};
	vec4_t       grey = { 0.2f, 0.2f, 0.2f, 1.0f };
	glyphInfo_t  *glyph;
	float		 useScale;
	fontInfo_t	 *font = &cls.font;

	useScale = scale * font->glyphScale;

	if (text) {
		const char	*s = text;
		re.SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
		len = strlen(text);

		count = 0;

		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s];

			if (Q_IsColorString(s)) {
				memcpy( newColor, g_color_table[ColorIndex(*(s + 1))], sizeof(newColor));
				newColor[3] = color[3];
				re.SetColor( newColor );
				s += 2;
				continue;
			}

			float  yadj = useScale * glyph->top;

			if ((style == ITEM_TEXTSTYLE_SHADOWED) || (style == ITEM_TEXTSTYLE_SHADOWEDLESS)) {
				black[3] = newColor[3];

				if (style == ITEM_TEXTSTYLE_SHADOWEDLESS)
					black[3] *= 0.7;

				if (newColor[0] == 0.0f && newColor[1] == 0.0f && newColor[2] == 0.0f) {
					grey[3] = black[3];
					re.SetColor(grey);
				} else {
					re.SetColor(black);
				}

				SCR_DrawFontChar(x + 1, y - yadj + 1,
						  glyph->imageWidth,
						  glyph->imageHeight,
						  useScale,
						  glyph->s,
						  glyph->t,
						  glyph->s2,
						  glyph->t2,
						  glyph->glyph);

				colorBlack[3] = 1.0;
				re.SetColor(newColor);
			}

			SCR_DrawFontChar(x, y - yadj,
					  glyph->imageWidth,
					  glyph->imageHeight,
					  useScale,
					  glyph->s,
					  glyph->t,
					  glyph->s2,
					  glyph->t2,
					  glyph->glyph);
			x += (glyph->xSkip * useScale);
			s++;
			count++;
		}
		re.SetColor(NULL);
	}
}




//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording( void ) {
	char	string[1024];
	int		pos;

	if (!clc.demorecording || clc.spDemoRecording)
		return;

	pos = FS_FTell(clc.demofile);

	int nameLen = strlen(clc.demoName) + 1;
	char *demoName = malloc(nameLen);
	COM_StripExtension(clc.demoName, demoName, nameLen);

	char *fmt;

	if (strlen(demoName) > 40)
		fmt = "^1[DEMO]^7 %.40s...: ^1%iKB";
	else
		fmt = "^1[DEMO]^7 %s: ^1%iKB";

	sprintf( string, fmt, demoName, pos / 1024);

	SCR_DrawFontText(320 - SCR_FontWidth(string, 0.18) / 2, 10, 0.18, g_color_table[7], string, ITEM_TEXTSTYLE_SHADOWEDLESS);
}


/*
=================
SCR_DrawSpree
=================
*/
void SCR_DrawSpree( void ) {
	if (cl.snap.ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
		cl.snap.ps.pm_type > 4 ||
		cl_paused->value ||
		!cl_drawSpree->integer ||
		cl.snap.ps.clientNum != clc.clientNum ||
		!Cvar_VariableIntegerValue("cg_draw2d"))
		return;

	char killStr[12];
	int x = 53;
	int y = 458;

	if (Cvar_VariableValue("cg_crosshairNamesType") == 0) {
		y = 448;
	}

	if (cl_drawSpree->integer == 1 || cl_drawSpree->integer == 3) {
		Com_sprintf(killStr, 12, "K:^2%i", cl.currentKills);
		SCR_DrawCondensedString(x, y, 8, killStr, g_color_table[7], qfalse );
	}

	int spacing = 2;
	int size = 16;

	int width;
	int i;
	y = 440;

	if (cl_drawSpree->integer > 1) {
		if (cl.currentKills < 6) {
			width = size * cl.currentKills + spacing * (cl.currentKills - 1);
			x = 320 - width / 2;

			for (i = 0; i < cl.currentKills; i++) {
				SCR_DrawNamedPic(x, 450, size, size, "skull.tga");
				x += spacing + size;
			}
		} else {
			SCR_DrawNamedPic(304, 450, size, size, "skull.tga");
			SCR_DrawCondensedString(321, 456, 8, va("x%i", cl.currentKills), g_color_table[7], qfalse);
		}
	}
}

/*
=================
SCR_DrawClock
=================
*/
void SCR_DrawClock(void) {
	int 		color, fontsize, posx, posy, hour, minute, second;
	char 		string[16];
	qtime_t		now;

	color = cl_drawclockcolor->integer;
	fontsize = cl_drawclockfontsize->integer;
	posx = cl_drawclockposx->integer;
	posy = cl_drawclockposy->integer;

	if (cl_drawclock->integer) {
		Com_RealTime(&now);
		hour	= now.tm_hour;
		minute	= now.tm_min;
		second	= now.tm_sec;

		Com_sprintf(string, sizeof(string), "%02i:%02i:%02i", hour, minute, second);
		SCR_DrawCondensedString(posx * 10, posy * 10, fontsize, string, g_color_table[color], qtrue);
	}
}

/*
=================
SCR_PersistentCrosshair
=================
*/
void SCR_PersistentCrosshair(void) {
	if (cl.snap.ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ||
		cl.snap.ps.pm_type > 4 ||
		!Cvar_VariableIntegerValue("cg_draw2d"))
		return;

	int size = Cvar_VariableIntegerValue("cg_crosshairsize");

	SCR_DrawNamedPic(320 - size/2, 240 - size/2, size, size, "gfx/crosshairs/static/dot.tga");
}

/*
===============================================================================

DEBUG GRAPH

===============================================================================
*/

typedef struct
{
	float	value;
	int		color;
} graphsamp_t;

static	int			current;
static	graphsamp_t	values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color)
{
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void)
{
	int		a, x, y, w, i, h;
	float	v;
	int		color;

	//
	// draw the graph
	//
	w = cls.glconfig.vidWidth;
	x = 0;
	y = cls.glconfig.vidHeight;
	re.SetColor( g_color_table[0] );
	re.DrawStretchPic(x, y - cl_graphheight->integer, 
		w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader );
	re.SetColor( NULL );

	for (a=0 ; a<w ; a++)
	{
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v * cl_graphscale->integer + cl_graphshift->integer;
		
		if (v < 0)
			v += cl_graphheight->integer * (1+(int)(-v / cl_graphheight->integer));
		h = (int)v % cl_graphheight->integer;
		re.DrawStretchPic( x+w-1-a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader );
	}
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init( void ) {
	cl_timegraph = Cvar_Get ("timegraph", "0", CVAR_CHEAT);
	cl_debuggraph = Cvar_Get ("debuggraph", "0", CVAR_CHEAT);
	cl_graphheight = Cvar_Get ("graphheight", "32", CVAR_CHEAT);
	cl_graphscale = Cvar_Get ("graphscale", "1", CVAR_CHEAT);
	cl_graphshift = Cvar_Get ("graphshift", "0", CVAR_CHEAT);

	cl_drawclock = Cvar_Get ("cl_drawclock", "0", CVAR_ARCHIVE);
	cl_drawclockcolor = Cvar_Get ("cl_drawclockcolor", "7", CVAR_ARCHIVE);
	cl_drawclockfontsize = Cvar_Get ("cl_drawclockfontsize", "8", CVAR_ARCHIVE);
	cl_drawclockposx = Cvar_Get ("cl_drawclockposx", "29", CVAR_ARCHIVE);
	cl_drawclockposy = Cvar_Get ("cl_drawclockposy", "1", CVAR_ARCHIVE);
	cl_drawSpree = Cvar_Get("cl_drawspree", "0", CVAR_ARCHIVE);

	scr_initialized = qtrue;
}


//=======================================================

/*
==================
SCR_DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
void SCR_DrawScreenField( stereoFrame_t stereoFrame ) {
	re.BeginFrame( stereoFrame );

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	if ( cls.state != CA_ACTIVE && cls.state != CA_CINEMATIC ) {
		if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 ) {
			re.SetColor( g_color_table[0] );
			re.DrawStretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
			re.SetColor( NULL );
		}
	}

	if ( !uivm ) {
		Com_DPrintf("draw screen without UI loaded\n");
		return;
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	if ( !VM_Call( uivm, UI_IS_FULLSCREEN )) {
		switch( cls.state ) {
		default:
			Com_Error( ERR_FATAL, "SCR_DrawScreenField: bad cls.state" );
			break;
		case CA_CINEMATIC:
			SCR_DrawCinematic();
			break;
		case CA_DISCONNECTED:
			// force menu up
			S_StopAllSounds();
			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			VM_Call( uivm, UI_REFRESH, cls.realtime );
			VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qfalse );
			break;
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			CL_CGameRendering( stereoFrame );

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			// refresh to update the time
			VM_Call( uivm, UI_REFRESH, cls.realtime );
			VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qtrue );
			break;
		case CA_ACTIVE:
			CL_CGameRendering( stereoFrame );
			SCR_DrawDemoRecording();
			if ((cl.sniperDrawn == 1) && (cl_persistentcrosshair->integer)) {
				SCR_PersistentCrosshair();
			}
			SCR_DrawClock();
			SCR_DrawSpree();
			break;
		}
	}

	// the menu draws next
	if ( cls.keyCatchers & KEYCATCH_UI && uivm ) {
		VM_Call( uivm, UI_REFRESH, cls.realtime );
	}

	// console draws next
	Con_DrawConsole ();

	// debug graph can be drawn on top of anything
	if ( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer ) {
		SCR_DrawDebugGraph ();
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen( void ) {
	static int	recursive;

	if ( !scr_initialized ) {
		return;				// not initialized yet
	}

	if ( ++recursive > 2 ) {
		Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
	}
	recursive = 1;

	// if running in stereo, we need to draw the frame twice
	if ( cls.glconfig.stereoEnabled ) {
		SCR_DrawScreenField( STEREO_LEFT );
		SCR_DrawScreenField( STEREO_RIGHT );
	} else {
		SCR_DrawScreenField( STEREO_CENTER );
	}

	if ( com_speeds->integer ) {
		re.EndFrame( &time_frontend, &time_backend );
	} else {
		re.EndFrame( NULL, NULL );
	}

	recursive = 0;
}

