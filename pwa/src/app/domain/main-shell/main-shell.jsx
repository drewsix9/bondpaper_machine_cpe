import { useEffect, useState } from "react";
import { Outlet, useLocation, useNavigate } from "react-router-dom";
import { INACTIVITY_CONFIG } from "../../shared/inactivity-config";

export default function MainShell() {
  const navigate = useNavigate();
  const location = useLocation();
  const [lastActivity, setLastActivity] = useState(Date.now());

  // Check if we're on the welcome page (root path)
  const isWelcomePage = location.pathname === "/" || location.pathname === "";

  // Reset the inactivity timer
  const resetInactivityTimer = () => {
    setLastActivity(Date.now());
  };

  // Check for inactivity and redirect if needed
  useEffect(() => {
    // Don't run the inactivity timer on the welcome page
    if (isWelcomePage) {
      return;
    }

    // console.log("Inactivity timer active - not on welcome page");

    const timer = setInterval(() => {
      const now = Date.now();
      const timeElapsed = now - lastActivity;

      if (timeElapsed >= INACTIVITY_CONFIG.timeout) {
        // Redirect to welcome page if inactive for too long
        console.log(
          "User inactive for",
          Math.floor(timeElapsed / 1000),
          "seconds - redirecting to welcome page"
        );
        navigate("/");
        // Reset the timer after redirect
        resetInactivityTimer();
      }
    }, 1000); // Check every second

    return () => {
      clearInterval(timer);
    };
  }, [lastActivity, navigate, isWelcomePage]);

  // Set up event listeners for user activity
  useEffect(() => {
    // Don't set up event listeners on the welcome page
    if (isWelcomePage) {
      console.log("On welcome page - inactivity timer disabled");
      return;
    }

    // List of events to track for user activity
    const events = [
      "mousedown",
      "mousemove",
      "keypress",
      "scroll",
      "touchstart",
      "click",
    ];

    // Event handler function
    const handleUserActivity = () => {
      resetInactivityTimer();
    };

    // Add all event listeners
    events.forEach((event) => {
      window.addEventListener(event, handleUserActivity);
    });

    // Clean up event listeners
    return () => {
      events.forEach((event) => {
        window.removeEventListener(event, handleUserActivity);
      });
    };
  }, [isWelcomePage]);

  return (
    <div className="h-screen bg-gray-100">
      <Outlet />
    </div>
  );
}
