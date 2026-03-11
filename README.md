# Web Browser Backend

Un serveur HTTP léger écrit en **C** qui simule le backend d'un navigateur web multi-onglets. Il gère les onglets, l'historique de navigation et la persistance de l'état via un fichier JSON.

---

## Description

Ce projet implémente une API REST en C pur, utilisant des sockets POSIX bruts (`sys/socket.h`). Chaque onglet possède son propre historique de navigation (liste doublement chaînée), et l'état complet est sauvegardé automatiquement dans un fichier `browser_state.json`.

---

## Structure du projet

```
.
├──browser
  ├── browser.h           # Structures de données et prototypes
  ├── browser.c    # Implementation de la logique métier (onglets, historique)
├──serialize
  ├── json-serialize.h     # Prototypes pour la sérialisation JSON
  ├── json-serialize.c     # Implementation de la sérialisation JSON
├──state
  ├── state.h              # Prototypes pour la gestion de l'état global
  ├── state.c              # Implementation de la gestion de l'état global (chargement/sauvegarde)
├── server.c             # Serveur HTTP et gestion des requêtes
├── Makefile            # Compilation du projet et réinitialisation
├── browser_state.json  # Fichier d'état généré automatiquement (persistance)
├── mock_data.json      # Fichier de données de test (exemples d'onglets et d'historique) 
```

---

## Prérequis

- **GCC** (ou tout compilateur C compatible)
- **libjson-c** — bibliothèque JSON pour C

### Installation de `json-c` (Ubuntu/Debian)

```bash
sudo apt install libjson-c-dev
```

### Installation de `json-c` (macOS avec Homebrew)

```bash
brew install json-c
```

---

## Compilation et lancement

```bash
# Compiler le projet
make

# Ajouter les données de test (optionnel)
touch browser_state.json
cp mock_data.json browser_state.json

# Lancer le serveur
./server
```

Le serveur écoute sur **http://localhost:3001**.

Pour réinitialiser l'état du simulateur :

```bash
make clean
```

---

## API REST

Le serveur expose les endpoints suivants :

| Méthode  | Endpoint                     | Description                                  |
|----------|------------------------------|----------------------------------------------|
| `GET`    | `/tabs`                      | Récupérer tous les onglets                   |
| `POST`   | `/tabs`                      | Créer un nouvel onglet                       |
| `DELETE` | `/tab/:id`                   | Fermer un onglet                             |
| `POST`   | `/tab/:id/visit`             | Visiter une URL dans un onglet               |
| `POST`   | `/tab/:id/navigate`          | Naviguer en arrière (`back`) ou en avant (`forward`) |

### Exemples de requêtes

**Visiter une URL :**
```bash
curl -X POST http://localhost:3001/tab/1/visit \
  -H "Content-Type: application/json" \
  -d '{"url": "https://example.com"}'
```

**Naviguer en arrière :**
```bash
curl -X POST http://localhost:3001/tab/1/navigate \
  -H "Content-Type: application/json" \
  -d '{"direction": "back"}'
```

**Récupérer tous les onglets :**
```bash
curl http://localhost:3001/tabs
```

---

## Persistance de l'état

L'état de tous les onglets est automatiquement sauvegardé dans `browser_state.json` après chaque modification. Au démarrage, le serveur recharge l'état depuis ce fichier si celui-ci existe.

---

## Fonctionnalités

- Gestion multi-onglets
- Historique de navigation par onglet (liste doublement chaînée)
- Navigation avant / arrière
- Persistance automatique de l'état (JSON)
- Support CORS pour les requêtes cross-origin

---

## Frontend

> **Note :** Une interface web (frontend) est disponible pour interagir visuellement avec ce backend :
>
> [**Oussama-Najih/web_browser_frontend**](https://github.com/Oussama-Najih/web_browser_frontend)
>
> Ce frontend est développé avec **Next.js** (React / TypeScript) et se connecte à ce serveur sur `http://localhost:3001`.
>
> Pour le lancer :
> ```bash
> cd web_browser_frontend
> npm install
> npm run dev
> ```
> Puis ouvrir [http://localhost:3000](http://localhost:3000).

---

## Contexte

Ce projet représente le projet de fin de ma deuxième année et se concentre sur l’implémentation de structures de données en langage C.
