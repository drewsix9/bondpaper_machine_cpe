import { useState } from "react";
import { NavLink, Outlet } from "react-router-dom";

export default function System() {
  const [activeMenu, setActiveMenu] = useState();
  const navigationItems = [
    {
      path: "paper",
      label: "Paper",
      icon: null,
    },
    {
      path: "quantity",
      label: "Quantity",
      icon: null,
    },
    {
      path: "pay",
      label: "Pay Amount",
      icon: null,
    },
    {
      path: "done",
      label: "Done",
      icon: null,
    },
  ];

  return (
    <div className="flex h-full">
      <div className="space-y-5 bg-white h-full w-48 p-4 shadow-sm">
        {navigationItems.map((item) => (
          <div className="flex flex-col" key={item.path}>
            <NavLink
              id={item.path}
              to={item.path}
              className={({ isActive }) =>
                `${
                  isActive ? "bg-blue-500 text-white" : "bg-gray-200 text-black"
                } py-6`
              }
              onClick={(event) => setActiveMenu(event.target.id)}
            >
              <div className="flex items-center space-x-4 pointer-events-none">
                <span className="text-2xl">{item.icon}</span>
                <span>{item.label}</span>
              </div>
            </NavLink>
          </div>
        ))}
      </div>
      <div className="p-10 flex-1 flex justify-between flex-col">
        <div className="flex justify-center items-center">
          <Outlet />
        </div>
        {/* {
          <div className="flex justify-end">
            <div className="flex space-x-5">
              <button className="bg-blue-500 text-white px-14 py-4 w-full rounded-md">
                Cancel
              </button>
              <button className="bg-blue-500 text-white px-14 py-4 w-full rounded-md">
                Next
              </button>
            </div>
          </div>
        } */}
      </div>
    </div>
  );
}
