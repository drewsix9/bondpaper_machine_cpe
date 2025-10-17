import { Outlet, useNavigate } from "react-router-dom";

export default function MainShell() {
  return (
    <div className="h-screen bg-gray-100">
      <Outlet />
    </div>
  );
}
