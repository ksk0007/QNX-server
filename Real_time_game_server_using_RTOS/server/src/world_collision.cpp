#include "world_collision.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

namespace
{
    struct Point { float x; float y; };
    struct Polygon { const Point* points; int count; };

    static const Point path_0[76] = {
        { 61.0000f, -40.0000f },
        { 61.0000f, -28.0000f },
        { 70.0000f, -28.0000f },
        { 70.0000f, -25.0000f },
        { 79.0000f, -25.0000f },
        { 79.0000f, -20.0000f },
        { 105.0000f, -20.0000f },
        { 105.0000f, -8.0000f },
        { 108.0000f, -8.0000f },
        { 108.0000f, 10.0000f },
        { 91.0000f, 10.0000f },
        { 91.0000f, -8.0000f },
        { 79.0000f, -8.0000f },
        { 79.0000f, -6.0000f },
        { 70.0000f, -6.0000f },
        { 70.0000f, 0.0000f },
        { 65.0000f, 0.0000f },
        { 65.0000f, 1.0000f },
        { 61.0000f, 1.0000f },
        { 61.0000f, 3.0000f },
        { 53.0000f, 3.0000f },
        { 53.0000f, -0.3750f },
        { 53.1875f, -0.7500f },
        { 53.6250f, -1.0000f },
        { 54.0000f, -1.0000f },
        { 54.0000f, -1.3750f },
        { 54.1875f, -1.7500f },
        { 54.6250f, -2.0000f },
        { 55.0000f, -2.0000f },
        { 55.0000f, -2.3750f },
        { 55.1875f, -2.7500f },
        { 55.6250f, -3.0000f },
        { 56.0000f, -3.0000f },
        { 56.0000f, -7.0000f },
        { 53.0000f, -7.0000f },
        { 53.0000f, -8.0000f },
        { 56.0000f, -8.0000f },
        { 56.0000f, -11.0000f },
        { 53.0000f, -11.0000f },
        { 53.0000f, -12.0000f },
        { 52.0000f, -12.0000f },
        { 52.0000f, -14.0000f },
        { 42.0000f, -14.0000f },
        { 42.0000f, -15.0000f },
        { 30.0000f, -15.0000f },
        { 30.0000f, -14.0000f },
        { 20.0000f, -14.0000f },
        { 20.0000f, -13.6250f },
        { 19.8125f, -13.2500f },
        { 19.3750f, -13.0000f },
        { 19.0000f, -13.0000f },
        { 19.0000f, -9.0000f },
        { 19.6250f, -9.0000f },
        { 20.0000f, -8.5000f },
        { 20.0000f, -8.0000f },
        { 22.6250f, -8.0000f },
        { 23.0000f, -7.5000f },
        { 23.0000f, -5.0000f },
        { 20.0000f, -5.0000f },
        { 20.0000f, -2.0000f },
        { 9.0000f, -2.0000f },
        { 9.0000f, -1.0000f },
        { 5.0000f, -1.0000f },
        { 5.0000f, -3.0000f },
        { -3.0000f, -3.0000f },
        { -3.0000f, 0.0000f },
        { -2.3750f, 0.0000f },
        { -2.0000f, 0.5000f },
        { -2.0000f, 1.0000f },
        { 0.6250f, 1.0000f },
        { 1.0000f, 1.5000f },
        { 1.0000f, 4.0000f },
        { -6.0000f, 4.0000f },
        { -6.0000f, 18.0000f },
        { -18.0000f, 18.0000f },
        { -18.0000f, -40.0000f },
    };

    static const Point path_1[62] = {
        { 29.6250f, -11.0000f },
        { 30.0000f, -10.5000f },
        { 30.0000f, -10.0000f },
        { 47.0000f, -10.0000f },
        { 47.0000f, -10.3750f },
        { 47.1875f, -10.7500f },
        { 47.6250f, -11.0000f },
        { 50.6250f, -11.0000f },
        { 51.0000f, -10.5000f },
        { 51.0000f, -9.0000f },
        { 48.0000f, -9.0000f },
        { 48.0000f, -6.0000f },
        { 51.0000f, -6.0000f },
        { 51.0000f, -5.0000f },
        { 48.0000f, -5.0000f },
        { 48.0000f, -4.0000f },
        { 47.0000f, -4.0000f },
        { 47.0000f, -9.0000f },
        { 30.0000f, -9.0000f },
        { 30.0000f, -3.0000f },
        { 31.0000f, -3.0000f },
        { 31.0000f, -4.0000f },
        { 47.0000f, -4.0000f },
        { 47.0000f, -3.0000f },
        { 46.0000f, -3.0000f },
        { 46.0000f, -1.0000f },
        { 44.0000f, -1.0000f },
        { 44.0000f, -2.0000f },
        { 36.0000f, -2.0000f },
        { 36.0000f, -1.0000f },
        { 32.0000f, -1.0000f },
        { 32.0000f, 0.0000f },
        { 32.6250f, 0.0000f },
        { 33.0000f, 0.5000f },
        { 33.0000f, 1.0000f },
        { 48.0000f, 1.0000f },
        { 48.0000f, 8.0000f },
        { 48.6250f, 8.0000f },
        { 49.0000f, 8.5000f },
        { 49.0000f, 9.0000f },
        { 52.6250f, 9.0000f },
        { 53.0000f, 9.5000f },
        { 53.0000f, 10.0000f },
        { 61.6250f, 10.0000f },
        { 62.0000f, 10.5000f },
        { 62.0000f, 12.0000f },
        { 45.0000f, 12.0000f },
        { 45.0000f, 11.0000f },
        { 44.0000f, 11.0000f },
        { 44.0000f, 10.0000f },
        { 43.0000f, 10.0000f },
        { 43.0000f, 8.0000f },
        { 47.0000f, 8.0000f },
        { 47.0000f, 2.0000f },
        { 30.0000f, 2.0000f },
        { 30.0000f, 0.0000f },
        { 29.0000f, 0.0000f },
        { 29.0000f, -9.0000f },
        { 25.0000f, -9.0000f },
        { 25.0000f, -10.3750f },
        { 25.1875f, -10.7500f },
        { 25.6250f, -11.0000f },
    };

    static const Point path_2[28] = {
        { 28.0000f, 1.0000f },
        { 28.0000f, 3.0000f },
        { 27.0000f, 3.0000f },
        { 27.0000f, 4.0000f },
        { 26.0000f, 4.0000f },
        { 26.0000f, 5.0000f },
        { 22.0000f, 5.0000f },
        { 22.0000f, 7.0000f },
        { 20.0000f, 7.0000f },
        { 20.0000f, 5.6250f },
        { 20.1875f, 5.2500f },
        { 20.6250f, 5.0000f },
        { 21.0000f, 5.0000f },
        { 21.0000f, 4.6250f },
        { 21.1875f, 4.2500f },
        { 21.6250f, 4.0000f },
        { 25.0000f, 4.0000f },
        { 25.0000f, 3.6250f },
        { 25.1875f, 3.2500f },
        { 25.6250f, 3.0000f },
        { 26.0000f, 3.0000f },
        { 26.0000f, 2.6250f },
        { 26.1875f, 2.2500f },
        { 26.6250f, 2.0000f },
        { 27.0000f, 2.0000f },
        { 27.0000f, 1.6250f },
        { 27.1875f, 1.2500f },
        { 27.6250f, 1.0000f },
    };

    static const Point path_3[6] = {
        { 29.0000f, 0.0000f },
        { 29.0000f, 1.0000f },
        { 28.0000f, 1.0000f },
        { 28.0000f, 0.6250f },
        { 28.1875f, 0.2500f },
        { 28.6250f, 0.0000f },
    };

    static const Point path_4[6] = {
        { 51.8750f, 0.0000f },
        { 52.0000f, 0.1250f },
        { 52.0000f, 1.0000f },
        { 51.0000f, 1.0000f },
        { 51.0000f, 0.1250f },
        { 51.0625f, 0.0000f },
    };

    static const Point path_5[6] = {
        { 49.8750f, -2.0000f },
        { 50.0000f, -1.8750f },
        { 50.0000f, -1.0000f },
        { 49.0000f, -1.0000f },
        { 49.0000f, -1.8750f },
        { 49.0625f, -2.0000f },
    };

    static const Polygon world_polygons[] = {
        { path_0, 76 },
        { path_1, 62 },
        { path_2, 28 },
        { path_3, 6 },
        { path_4, 6 },
        { path_5, 6 },
    };
    static const int WORLD_POLYGON_COUNT = sizeof(world_polygons) / sizeof(world_polygons[0]);

    static const float HALF_WIDTH  = 0.6600f * 0.5f;
    static const float HALF_HEIGHT = 1.62529f * 0.5f;
    static const float OFFSET_X = -0.007f;
    static const float OFFSET_Y = -0.234f;
    static const float EPSILON = 0.001f;

    static float Min4(float a, float b, float c, float d)
    { return std::min(std::min(a,b), std::min(c,d)); }
    static float Max4(float a, float b, float c, float d)
    { return std::max(std::max(a,b), std::max(c,d)); }

    static bool PointInPolygon(float x, float y, const Point* p, int n)
    {
        bool inside = false;
        for (int i = 0, j = n - 1; i < n; j = i++)
        {
            const float xi=p[i].x, yi=p[i].y;
            const float xj=p[j].x, yj=p[j].y;
            const float cross = (x-xi)*(yj-yi) - (y-yi)*(xj-xi);
            const float minx=std::min(xi,xj), maxx=std::max(xi,xj);
            const float miny=std::min(yi,yj), maxy=std::max(yi,yj);
            if (std::fabs(cross) < 0.00001f && x >= minx-EPSILON && x <= maxx+EPSILON && y >= miny-EPSILON && y <= maxy+EPSILON)
                return true;
            const bool crosses = ((yi > y) != (yj > y));
            if (crosses)
            {
                const float xcross = xi + (y-yi) * (xj-xi) / (yj-yi);
                if (x < xcross) inside = !inside;
            }
        }
        return inside;
    }

    static int Orientation(float ax,float ay,float bx,float by,float cx,float cy)
    {
        const float v=(by-ay)*(cx-bx)-(bx-ax)*(cy-by);
        if (std::fabs(v)<0.00001f) return 0;
        return v>0.0f ? 1 : 2;
    }
    static bool OnSegment(float ax,float ay,float bx,float by,float cx,float cy)
    {
        return bx >= std::min(ax,cx)-EPSILON && bx <= std::max(ax,cx)+EPSILON &&
               by >= std::min(ay,cy)-EPSILON && by <= std::max(ay,cy)+EPSILON;
    }
    static bool SegmentsIntersect(float ax,float ay,float bx,float by,float cx,float cy,float dx,float dy)
    {
        const int o1=Orientation(ax,ay,bx,by,cx,cy);
        const int o2=Orientation(ax,ay,bx,by,dx,dy);
        const int o3=Orientation(cx,cy,dx,dy,ax,ay);
        const int o4=Orientation(cx,cy,dx,dy,bx,by);
        if (o1 != o2 && o3 != o4) return true;
        if (o1==0 && OnSegment(ax,ay,cx,cy,bx,by)) return true;
        if (o2==0 && OnSegment(ax,ay,dx,dy,bx,by)) return true;
        if (o3==0 && OnSegment(cx,cy,ax,ay,dx,dy)) return true;
        if (o4==0 && OnSegment(cx,cy,bx,by,dx,dy)) return true;
        return false;
    }

    static bool PointInBox(float x,float y,float minx,float maxx,float miny,float maxy)
    { return x >= minx-EPSILON && x <= maxx+EPSILON && y >= miny-EPSILON && y <= maxy+EPSILON; }

    static bool PolygonOverlapsBox(const Point* p,int n,float minx,float maxx,float miny,float maxy)
    {
        float pminx=p[0].x,pmaxx=p[0].x,pminy=p[0].y,pmaxy=p[0].y;
        for(int i=1;i<n;i++){pminx=std::min(pminx,p[i].x);pmaxx=std::max(pmaxx,p[i].x);pminy=std::min(pminy,p[i].y);pmaxy=std::max(pmaxy,p[i].y);}
        if(pmaxx < minx || pminx > maxx || pmaxy < miny || pminy > maxy) return false;
        if(PointInPolygon(minx,miny,p,n) || PointInPolygon(maxx,miny,p,n) || PointInPolygon(maxx,maxy,p,n) || PointInPolygon(minx,maxy,p,n)) return true;
        for(int i=0;i<n;i++){ const Point& a=p[i]; const Point& b=p[(i+1)%n];
            if(PointInBox(a.x,a.y,minx,maxx,miny,maxy)) return true;
            if(SegmentsIntersect(a.x,a.y,b.x,b.y,minx,miny,maxx,miny) ||
               SegmentsIntersect(a.x,a.y,b.x,b.y,maxx,miny,maxx,maxy) ||
               SegmentsIntersect(a.x,a.y,b.x,b.y,maxx,maxy,minx,maxy) ||
               SegmentsIntersect(a.x,a.y,b.x,b.y,minx,maxy,minx,miny)) return true;
        }
        return false;
    }

    static bool OverlapsAt(const Player* player,float x,float y)
    {
        const float cx=x+OFFSET_X, cy=y+OFFSET_Y;
        const float minx=cx-HALF_WIDTH+EPSILON, maxx=cx+HALF_WIDTH-EPSILON;
        const float miny=cy-HALF_HEIGHT+EPSILON, maxy=cy+HALF_HEIGHT-EPSILON;
        for(int i=0;i<WORLD_POLYGON_COUNT;i++)
            if(PolygonOverlapsBox(world_polygons[i].points,world_polygons[i].count,minx,maxx,miny,maxy)) return true;
        return false;
    }

    static float SafeAxisMove(const Player* player,float start_x,float start_y,float desired_x,float desired_y,bool horizontal)
    {
        const float start = horizontal ? start_x : start_y;
        const float end   = horizontal ? desired_x : desired_y;
        if(!OverlapsAt(player,start_x,start_y))
        {
            if(!OverlapsAt(player,desired_x,desired_y)) return end;
            float lo=0.0f, hi=1.0f;
            for(int i=0;i<12;i++){
                const float t=(lo+hi)*0.5f;
                const float tx=horizontal ? start_x+(desired_x-start_x)*t : desired_x;
                const float ty=horizontal ? desired_y : start_y+(desired_y-start_y)*t;
                if(OverlapsAt(player,tx,ty)) hi=t; else lo=t;
            }
            const float t=lo;
            return start+(end-start)*t;
        }
        return start;
    }
}

void WorldCollision_Init()
{
    std::printf("[COLLISION] World geometry loaded: %d polygons, %d points\n", WORLD_POLYGON_COUNT,
        (int)(sizeof(path_0)/sizeof(path_0[0]) + sizeof(path_1)/sizeof(path_1[0]) + sizeof(path_2)/sizeof(path_2[0]) + sizeof(path_3)/sizeof(path_3[0]) + sizeof(path_4)/sizeof(path_4[0]) + sizeof(path_5)/sizeof(path_5[0])));
    std::printf("[COLLISION] Player collider approximation: W=%.5f H=%.5f Offset=(%.5f, %.5f)\n", HALF_WIDTH*2.0f, HALF_HEIGHT*2.0f, OFFSET_X, OFFSET_Y);
}

bool WorldCollision_PlayerOverlaps(const Player* player)
{
    if(player==NULL) return false;
    return OverlapsAt(player,player->x,player->y);
}

void WorldCollision_MovePlayer(Player* player,float delta_x,float delta_y)
{
    if(player==NULL) return;

    /* If a spawn point starts inside the tilemap, lift the player until
       the collider is clear. This is a safety net for old spawn coordinates. */
    if(OverlapsAt(player, player->x, player->y))
    {
        const float original_y = player->y;
        bool cleared = false;

        for(int step=1; step<=400; ++step)
        {
            const float test_y = original_y + (float)step * 0.05f;
            if(!OverlapsAt(player, player->x, test_y))
            {
                player->y = test_y;
                player->velocity_y = 0.0f;
                player->is_grounded = true;
                cleared = true;
                break;
            }
        }

        if(!cleared)
        {
            player->velocity_y = 0.0f;
            player->is_grounded = false;
        }
    }

    const float old_x=player->x, old_y=player->y;
    /* Resolve X first: this stops the player against walls. */
    const float wanted_x=old_x+delta_x;
    player->x=SafeAxisMove(player,old_x,old_y,wanted_x,old_y,true);
    if(std::fabs(player->x-wanted_x)>0.0001f) player->input_x=0.0f;
    /* Resolve Y second: this stops on floors, platforms and ceilings. */
    const float wanted_y=old_y+delta_y;
    const float resolved_y=SafeAxisMove(player,player->x,old_y,player->x,wanted_y,false);
    if(std::fabs(resolved_y-wanted_y)>0.0001f)
    {
        if(delta_y < 0.0f) player->is_grounded=true;
        player->velocity_y=0.0f;
    }
    else
    {
        player->is_grounded=false;
    }
    player->y=resolved_y;
}
