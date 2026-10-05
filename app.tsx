function AboutMe() {
  return (
    <section>
      <h2>Особиста інформація</h2>
      <p><strong>ПІБ:</strong> Глєб</p>
      <p><strong>Контактний телефон:</strong> +380 XX XXX XX XX</p>
      <p><strong>Електронна адреса:</strong> gleb@example.com</p>
    </section>
  );
}

export default function App() {
  return (
    <main style={{ padding: '20px', fontFamily: 'sans-serif' }}>
      <h1>Завдання 1: Інформація про себе</h1>
      <AboutMe />
    </main>
  );
}
