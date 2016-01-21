//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2016.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  This file defines classes to represent bodies that exist on a 2D surface.
//  Each class should be able to:
//   * Maintain a pointer to information about the full organism associated with this body.
//   * provide a circular perimeter of the body (for phase1 of collision detection)
//   * Provide body an anchor point and center point of the body (typically the same)
//
//  Currently, the only type of body we have is:
//
//    CircleBody2D - One individual circular object in the 2D world.
//
//
//  Development notes:
//  * Links should probably be shared by both bodies.
//  * If we are going to have a lot of links, we may want a better data structure than vector.


#ifndef EMP_BODY_2D_H
#define EMP_BODY_2D_H

#include "../tools/assert.h"
#include "../tools/alert.h"
#include "../tools/mem_track.h"
#include "../tools/vector.h"

#include "Angle2D.h"
#include "Circle2D.h"

namespace emp {

  class Body2D_Base {
  protected:
    enum class LINK_TYPE { NOT_SET, REPRODUCTION, BOND, ATTACK, TARGET };
    
    template <typename BODY_TYPE>
    struct BodyLink {
      LINK_TYPE type;
      BODY_TYPE * other;
      double cur_dist;     // How far are bodies currently being kept apart?
      double target_dist;  // How far should the be moved to before splitting?

      BodyLink() : type(LINK_TYPE::NOT_SET), other(nullptr), cur_dist(0), target_dist(0) { ; }
      BodyLink(LINK_TYPE t, BODY_TYPE * o, double cur=0, double target=0)
        : type(t), other(o), cur_dist(cur), target_dist(target) { ; }
      BodyLink(const BodyLink &) = default;
      ~BodyLink() { ; }
    };

    double birth_time;        // At what time point was this organism born?
    Angle orientation;        // Which way is body facing?
    Point<double> velocity;   // Speed and direction of movement
    double mass;              // "Weight" of this object (@CAO not used yet..)
    uint32_t color_id;        // Which color should this body appear?
    int repro_count;          // Number of offspring currently being produced.

    Point<double> shift;            // How should this body be updated to minimize overlap.
    Point<double> cum_shift;        // Build up of shift not yet acted upon.
    Point<double> total_abs_shift;  // Total absolute-value of shifts (to calculate pressure)
    double pressure;                // Current pressure on this body.

  public:
    Body2D_Base() : birth_time(0.0), mass(1.0), color_id(0), repro_count(0), pressure(0) { ; }
    ~Body2D_Base() { ; }

    double GetBirthTime() const { return birth_time; }
    const Angle & GetOrientation() const { return orientation; }
    const Point<double> & GetVelocity() const { return velocity; }
    double GetMass() const { return mass; }
    uint32_t GetColorID() const { return color_id; }
    bool IsReproducing() const { return repro_count; }
    int GetReproCount() const { return repro_count; }
    Point<double> GetShift() const { return shift; }
    double GetPressure() const { return pressure; }
    
    
    void SetBirthTime(double in_time) { birth_time = in_time; }
    void SetColorID(uint32_t in_id) { color_id = in_id; }

    // Orientation control...
    void TurnLeft(int steps=1) { orientation.RotateDegrees(45); }
    void TurnRight(int steps=1) { orientation.RotateDegrees(-45); }

    // Velocity control...
    void IncSpeed(const Point<double> & offset) { velocity += offset; }
    void IncSpeed() { velocity += orientation.GetPoint<double>(); }
    void DecSpeed() { velocity -= orientation.GetPoint<double>(); }
    void SetVelocity(double x, double y) { velocity.Set(x, y); }
    void SetVelocity(const Point<double> & v) { velocity = v; }

    // Shift to apply next update.
    void AddShift(const Point<double> & s) { shift += s; total_abs_shift += s.Abs(); }
};
  
  class CircleBody2D : public Body2D_Base {
  private:
    Circle<double> perimeter;  // Includes position and size.
    double target_radius;      // For growing/shrinking
    
    // Information about other bodies that this one is linked to.
    emp::vector< BodyLink<CircleBody2D> > links;  // Active links
    emp::vector< CircleBody2D * > dead_links;     // List of links to remove!

  public:
    CircleBody2D(const Circle<double> & _p)
      : perimeter(_p), target_radius(_p.GetRadius())
    {
      EMP_TRACK_CONSTRUCT(CircleBody2D);
    }
    ~CircleBody2D() {
      // If this body is paired with another one, remove the pairing.
      if (links.size()) {
        for (auto & link : links) link.other->RemoveLink(*this, false);
        links.resize(0);
      }
      EMP_TRACK_DESTRUCT(CircleBody2D);
    }

    const Circle<double> & GetPerimeter() const { return perimeter; }
    const Point<double> & GetAnchor() const { return perimeter.GetCenter(); }
    const Point<double> & GetCenter() const { return perimeter.GetCenter(); }
    double GetRadius() const { return perimeter.GetRadius(); }
    double GetTargetRadius() const { return target_radius; }
    
    void SetPosition(const Point<double> & p) { perimeter.SetCenter(p); }
    void SetRadius(double r) { perimeter.SetRadius(r); }
    void SetTargetRadius(double t) { target_radius = t; }
    
    // Translate immediately (ignoring physics)
    void Translate(const Point<double> & t) { perimeter.Translate(t); }

    // Creating, testing, and unlinking other organisms
    bool IsLinked(const CircleBody2D & link_org) const {
      for (auto & cur_link : links) if (cur_link.other == &link_org) return true;
      return false;
    }

    int GetLinkCount() const { return (int) links.size(); }

    void AddLink(LINK_TYPE type, CircleBody2D & link_org, double cur_dist, double target_dist) {
      emp_assert(!IsLinked(link_org));  // Don't link twice!

      // Build connections in both directions.
      links.emplace_back(type, &link_org, cur_dist, target_dist);
      link_org.links.emplace_back(LINK_TYPE::TARGET, this, cur_dist, target_dist);
    }

    void RemoveLink(CircleBody2D & link_org, bool remove_link_back=true) {
      // Find the link and remove it.
      for (int i = 0; i < (int) links.size(); i++) {
        if (links[i].other == &link_org) {
          links[i] = links.back();
          links.pop_back();
          break;
        }
      }

      // Remove link in other direction (unless we don't need to).
      if (remove_link_back) link_org.RemoveLink(*this, false);
    }

    const BodyLink<CircleBody2D> & FindLink(const CircleBody2D & link_org) const {
      emp_assert(IsLinked(link_org));
      for (auto & link : links) if ( link.other == &link_org) return link;
      return links[0]; // Should never get here!
    }
    
    BodyLink<CircleBody2D> & FindLink(CircleBody2D & link_org)  {
      emp_assert(IsLinked(link_org));
      for (auto & link : links) if ( link.other == &link_org) return link;
      return links[0]; // Should never get here!
    }
    
    double GetLinkDist(const CircleBody2D & link_org) const {
      emp_assert(IsLinked(link_org));
      return FindLink(link_org).cur_dist;
    }
    double GetTargetLinkDist(const CircleBody2D & link_org) const {
      emp_assert(IsLinked(link_org));
      return FindLink(link_org).target_dist;
    }
    void ShiftLinkDist(CircleBody2D & link_org, double change) {
      auto & link = FindLink(link_org);
      auto & olink = link.other->FindLink(*this);
      
      link.cur_dist += change;
      olink.cur_dist = link.cur_dist;
    }

    CircleBody2D * BuildOffspring(emp::Point<double> offset) {
      // Offspring cannot be right on top of parent.
      emp_assert(offset.GetX() != 0 || offset.GetY() != 0);

      // Create the offspring as a paired link.
      auto * offspring = new CircleBody2D(perimeter);
      AddLink(LINK_TYPE::REPRODUCTION, *offspring, offset.Magnitude(), perimeter.GetRadius()*2.0);
      offspring->Translate(offset);
      repro_count++;
      
      return offspring;
    }

    // If a body is not at its target radius, grow it or shrink it, as needed.
    void BodyUpdate(double change_factor=1, bool detach_on_birth=true) {
      // Test if this body needs to grow or shrink.
      if ((int) target_radius > (int) GetRadius()) SetRadius(GetRadius() + change_factor);
      else if ((int) target_radius < (int) GetRadius()) SetRadius(GetRadius() - change_factor);

      // If there are any links flagged for removal, do so!
      if (dead_links.size()) {
        for (auto * other : dead_links) RemoveLink(*other);
        dead_links.resize(0);
      }

      // Test if the link distance for this body needs to be updated
      for (auto & link : links) {
        if (link.cur_dist != link.target_dist) {
          // If we're within the change_factor, just set pair_dist to target.
          if (std::abs(link.cur_dist - link.target_dist) <= change_factor) {
            link.cur_dist = link.target_dist;
            if (link.type == LINK_TYPE::REPRODUCTION) {
              emp_assert(repro_count > 0);
              repro_count--;
              if (detach_on_birth) dead_links.push_back(link.other);  // Flag link for removal!
            }
          }
          else {
            // @CAO because of the previous check, do we really need to case to int here??
            if ((int) link.cur_dist < (int) link.target_dist) link.cur_dist += change_factor;
            else link.cur_dist -= change_factor;
          }
        }
      }

    }


    // Move this body by its velocity and reduce velocity based on friction.
    void ProcessStep(double friction=0) {
      if (velocity.NonZero()) {
        perimeter.Translate(velocity);
        const double velocity_mag = velocity.Magnitude();

        // If body is close to stopping stop it!
        if (friction > velocity_mag) { velocity.ToOrigin(); }

        // Otherwise slow it down proportionately in the x and y directions.
        else { velocity *= 1.0 - ((double) friction) / ((double) velocity_mag); }
      }   
    }


    // Determine where the circle will end up and force it to be within a bounding box.
    void FinalizePosition(const Point<double> & max_coords) {
      const double max_x = max_coords.GetX() - GetRadius();
      const double max_y = max_coords.GetY() - GetRadius();

      // Update the caclulcation for pressure.

      // Act on the accumulated shifts only when they add up enough.
      cum_shift += shift;
      if (cum_shift.SquareMagnitude() > 0.25) {
        perimeter.Translate(cum_shift);
        cum_shift.ToOrigin();
      }
      pressure = (total_abs_shift - shift.Abs()).SquareMagnitude();
      shift.ToOrigin();              // Clear out the shift for the next round.
      total_abs_shift.ToOrigin();
        
      // If this body is linked to another, enforce the distance between them.
      for (auto & link : links) {
        emp_assert(link.other->IsLinked(*this));

        if (GetAnchor() == link.other->GetAnchor()) {
          // If two organisms are on top of each other... shift one.
          Translate(emp::Point<double>(0.01, 0.01));
        }
        
        // Figure out how much each oragnism should move so that they will be properly spaced.
        const double start_dist = GetAnchor().Distance(link.other->GetAnchor());
        const double link_dist = GetLinkDist(*(link.other));
        const double frac_change = (1.0 - ((double) link_dist) / ((double) start_dist)) / 2.0;
        
        emp::Point<double> dist_move = (GetAnchor() - link.other->GetAnchor()) * frac_change;
        
        perimeter.Translate(-dist_move);
        link.other->perimeter.Translate(dist_move);
      }
      
      // Adjust the organism so it stays within the bounding box of the world.
      if (GetCenter().GetX() < GetRadius()) { 
        perimeter.SetCenterX(GetRadius());     // Put back in range...
        velocity.NegateX();                    // Bounce off left side.
      } else if (GetCenter().GetX() > max_x) {
        perimeter.SetCenterX(max_x);           // Put back in range...
        velocity.NegateX();                    // Bounce off right side.
      }

      if (GetCenter().GetY() < GetRadius()) { 
        perimeter.SetCenterY(GetRadius());     // Put back in range...
        velocity.NegateY();                    // Bounce off top.
      } else if (GetCenter().GetY() > max_y) {
        perimeter.SetCenterY(max_y);           // Put back in range...
        velocity.NegateY();                    // Bounce off bottom.
      }
    }
    
    // Check to make sure there are no obvious issues with this object.
    bool OK() {
      for (auto & link : links) {
        (void) link;
        emp_assert(link.other->IsLinked(*this)); // Make sure pairing is reciprical.
        emp_assert(link.cur_dist >= 0);          // Distances cannot be negative.
        emp_assert(link.target_dist >= 0);       // Distances cannot be negative.
      }

      return true;
    }

  };
};

#endif
