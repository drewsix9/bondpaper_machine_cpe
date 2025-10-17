export default function FormattedCurrency({ amount }) {
  const formattedAmount = new Intl.NumberFormat("fil-PH", {
    style: "currency",
    currency: "PHP",
  }).format(amount);

  return <span>{formattedAmount}</span>;
}
