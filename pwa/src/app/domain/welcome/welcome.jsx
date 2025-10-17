import { useNavigate } from "react-router-dom";

export default function Welcome() {
  const navigate = useNavigate();

  return (
    <div className="flex justify-center items-center h-full">
      <div className="flex flex-col items-center justify-center space-y-5 bg-white h-4/5 w-1/3 rounded-md shadow-md px-5">
        <div>Logo</div>
        <h1>Paper Vending Machine</h1>
        <button
          className="bg-blue-500 text-white px-4 py-2 w-full rounded-md"
          onClick={() => navigate("system/paper")}
        >
          Get Started
        </button>
      </div>
    </div>
  );
}
