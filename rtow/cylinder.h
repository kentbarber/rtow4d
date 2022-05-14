#ifndef CYLINDER_H
#define CYLINDER_H

#include "rtweekend.h"

#include "aarect.h"
#include "hittable_list.h"

class cylinder : public hittable  {
    public:
        cylinder() {}
        cylinder(point3 cen, double top, double bot, double radius, shared_ptr<material> ptr)
            : m_cen(cen)
			, m_Top(top)
            , m_Bottom(bot)
            , m_Radius(radius)
            , m_InvRadius(1.0)
            , mat_ptr(ptr)
        {
            if (radius > 0)
            {
                m_InvRadius = 1.0 / radius;
            }
        }

        virtual ~cylinder() { }

        virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override;

        virtual bool bounding_box(double time0, double time1, aabb& output_box) const override {
            point3 box_max(m_Radius, m_Top, m_Radius);
			point3 box_min(-m_Radius, m_Bottom, -m_Radius);
            output_box = aabb(box_min, box_max);
            return true;
        }

    public:
		point3		m_cen;
		double		m_Top;
		double		m_Bottom;
		double		m_Radius;
		double		m_InvRadius;
        shared_ptr<material> mat_ptr;
};

static const double kEpsilon = 0.0001f;		///< @em 0.0001
inline bool cylinder::hit(const ray& r, double t_min, double t_max, hit_record& rec) const {

	double t;
	double t1;
	double t2;
	double ox = r.orig[0] - m_cen[0];
	double oy = r.orig[1] - m_cen[1];
	double oz = r.orig[2] - m_cen[2];
	double dx = r.dir[0];
	double dy = r.dir[1];
	double dz = r.dir[2];

	double dy_inv = 1.0 / dy;
	double rad2 = m_Radius * m_Radius;

	// Caps
	t1 = (m_Top - oy) * dy_inv;
	t2 = (m_Bottom - oy) * dy_inv;

	if (t1 > t2) { std::swap(t1, t2); }

	t = t1 < 0.0f ? t2 : t1;
	point3 p = r.at(t) - m_cen;

	double rad = (p[0] * p[0] + p[2] * p[2]);
	vec3 n(0, p[1], 0);

	if (rad < rad2 && dot(r.dir, n) < 0.0f) {

		if (t < t_min || t_max < t)
			return false;

		rec.t = t;
		rec.p = r.at(t);
		rec.set_face_normal(r, n);
		rec.mat_ptr = mat_ptr;
		return true;
	}

	// Sides
	double a = dx * dx + dz * dz;
	double b = 2.0 * (ox * dx + oz * dz);
	double c = ox * ox + oz * oz - rad2;
	double disc = b * b - 4.0 * a * c;

	if (disc < 0.0) 
		return false;

	double e = sqrt(disc);
	double denom = 2.0 * a;
	t1 = (-b - e) / denom;
	t2 = (-b + e) / denom;

	if (t1 > t2) 
		t = t2;
	else 
		t = t1;

	if (t < t_min || t_max < t)
		return false;

	if (t > kEpsilon) {
		double yhit = oy + t * dy;

		if (yhit > m_Bottom && yhit < m_Top) {
			rec.t = t;
			rec.p = r.at(rec.t);
			vec3 outward_normal((ox + t * dx) * m_InvRadius, 0.0, (oz + t * dz) * m_InvRadius);
			rec.set_face_normal(r, outward_normal);
			rec.mat_ptr = mat_ptr;
			return true;
		}
	}

	return false;
}


#endif
